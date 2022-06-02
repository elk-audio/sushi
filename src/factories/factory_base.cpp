/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
*
* SUSHI is free software: you can redistribute it and/or modify it under the terms of
* the GNU Affero General Public License as published by the Free Software Foundation,
* either version 3 of the License, or (at your option) any later version.
*
* SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
* PURPOSE. See the GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License along with
* SUSHI. If not, see http://www.gnu.org/licenses/
*/

#include "logging.h"

#include "factory_base.h"

#include "engine/audio_engine.h"
#include "engine/json_configurator.h"

#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/portaudio_frontend.h"

#include "audio_frontends/passive_frontend.h"
#include "control_frontends/passive_midi_frontend.h"

#include "audio_frontends/xenomai_raspa_frontend.h"

#include "control_frontends/osc_frontend.h"

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RT_MIDI
#include "control_frontends/rt_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "control_frontends/oscpack_osc_messenger.h"


namespace sushi
{

FactoryBase::FactoryBase() = default;

FactoryBase::~FactoryBase() = default;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi-factory");

InitStatus FactoryBase::_configure_from_file(sushi::SushiOptions& options)
{
    auto configurator = std::make_unique<sushi::jsonconfig::JsonConfigurator>(_engine.get(),
                                                                              _midi_dispatcher.get(),
                                                                              _engine->processor_container(),
                                                                              options.config_filename);

    auto [audio_config_status, audio_config] = configurator->load_audio_config();
    if (audio_config_status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        if (audio_config_status == sushi::jsonconfig::JsonConfigReturnStatus::INVALID_FILE)
        {
            return InitStatus::FAILED_INVALID_FILE_PATH;
        }

        return InitStatus::FAILED_INVALID_CONFIGURATION_FILE;
    }

    int midi_inputs = audio_config.midi_inputs.value_or(1);
    int midi_outputs = audio_config.midi_outputs.value_or(1);

#ifdef SUSHI_BUILD_WITH_RT_MIDI
    auto rt_midi_input_mappings = audio_config.rt_midi_input_mappings;
    auto rt_midi_output_mappings = audio_config.rt_midi_output_mappings;
#endif

    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    ///////////////////////////
    // Set up Audio Frontend //
    ///////////////////////////
    int cv_inputs = audio_config.cv_inputs.value_or(0);
    int cv_outputs = audio_config.cv_outputs.value_or(0);

    auto audio_frontend_status = _setup_audio_frontend(options, cv_inputs, cv_outputs);
    if (audio_frontend_status != InitStatus::OK)
    {
        return audio_frontend_status;
    }

    ////////////////////////
    // Load Configuration //
    ////////////////////////
    auto configuration_status = _load_json_configuration(options, configurator.get(), _audio_frontend.get());
    if (configuration_status != InitStatus::OK)
    {
        return configuration_status;
    }

    /////////////////////////////////////////////
    // Set up Controller and Control Frontends //
    /////////////////////////////////////////////
    auto control_status = _set_up_control(options, configurator.get(), midi_inputs, midi_outputs);
    if (control_status != InitStatus::OK)
    {
        return control_status;
    }

    return InitStatus::OK;
}

sushi::InitStatus FactoryBase::_configure_with_defaults(sushi::SushiOptions& options)
{
    int midi_inputs = 1;
    int midi_outputs = 1;

    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    int cv_inputs = 0;
    int cv_outputs = 0;

    auto status = _setup_audio_frontend(options, cv_inputs, cv_outputs);
    if (status != InitStatus::OK)
    {
        return status;
    }

    status = _set_up_control(options, nullptr, midi_inputs, midi_outputs);
    if (status != InitStatus::OK)
    {
        return status;
    }

    return InitStatus::OK;
}

sushi::InitStatus FactoryBase::_load_json_configuration(const sushi::SushiOptions& options,
                                                        sushi::jsonconfig::JsonConfigurator* configurator,
                                                        audio_frontend::BaseAudioFrontend* audio_frontend)
{
    auto status = configurator->load_host_config();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return InitStatus::FAILED_LOAD_HOST_CONFIG;
    }

    status = configurator->load_tracks();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return InitStatus::FAILED_LOAD_TRACKS;
    }

    status = configurator->load_midi();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK &&
        status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_MIDI_MAPPING;
    }

    status = configurator->load_cv_gate();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK &&
        status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_CV_GATE;
    }

    status = configurator->load_initial_state();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK &&
        status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_PROCESSOR_STATES;
    }

    if (options.frontend_type == FrontendType::DUMMY ||
        options.frontend_type == FrontendType::OFFLINE)
    {
        auto [status, events] = configurator->load_event_list();
        if (status == jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(audio_frontend)->add_sequencer_events(events);
        }
        else if (status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return InitStatus::FAILED_LOAD_EVENT_LIST;
        }
    }
    else
    {
        status = configurator->load_events();
        if (status != jsonconfig::JsonConfigReturnStatus::OK &&
            status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return InitStatus::FAILED_LOAD_EVENTS;
        }
    }

    return InitStatus::OK;
}

InitStatus FactoryBase::_setup_audio_frontend(const SushiOptions& options, int cv_inputs, int cv_outputs)
{
    switch (options.frontend_type)
    {
        case FrontendType::JACK:
            {
                SUSHI_LOG_INFO("Setting up Jack audio frontend");
                _frontend_config = std::make_unique<sushi::audio_frontend::JackFrontendConfiguration>(options.jack_client_name,
                                                                                                      options.jack_server_name,
                                                                                                      options.connect_ports,
                                                                                                      cv_inputs,
                                                                                                      cv_outputs);

                _audio_frontend = std::make_unique<sushi::audio_frontend::JackFrontend>(_engine.get());
                break;
            }

        case FrontendType::PORTAUDIO:
            {
                SUSHI_LOG_INFO("Setting up PortAudio frontend");
                _frontend_config = std::make_unique<sushi::audio_frontend::PortAudioFrontendConfiguration>(options.portaudio_input_device_id,
                                                                                                           options.portaudio_output_device_id,
                                                                                                           cv_inputs,
                                                                                                           cv_outputs);

                _audio_frontend = std::make_unique<sushi::audio_frontend::PortAudioFrontend>(_engine.get());
                break;
            }

        case FrontendType::XENOMAI_RASPA:
            {
                SUSHI_LOG_INFO("Setting up Xenomai RASPA frontend");
                _frontend_config = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontendConfiguration>(options.debug_mode_switches,
                                                                                                              cv_inputs,
                                                                                                              cv_outputs);

                _audio_frontend = std::make_unique<sushi::audio_frontend::XenomaiRaspaFrontend>(_engine.get());
                break;
            }
        case FrontendType::PASSIVE:
            {
                SUSHI_LOG_INFO("Setting up passive frontend");
                _frontend_config = std::make_unique<sushi::audio_frontend::PassiveFrontendConfiguration>(cv_inputs,
                                                                                                         cv_outputs);

                _audio_frontend = std::make_unique<sushi::audio_frontend::PassiveFrontend>(_engine.get());
                break;
            }
        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
            {
                bool dummy = false;

                if (options.frontend_type != FrontendType::OFFLINE)
                {
                    dummy = true;
                    SUSHI_LOG_INFO("Setting up dummy audio frontend");
                }
                else
                {
                    SUSHI_LOG_INFO("Setting up offline audio frontend");
                }

                _frontend_config = std::make_unique<sushi::audio_frontend::OfflineFrontendConfiguration>(options.input_filename,
                                                                                                         options.output_filename,
                                                                                                         dummy,
                                                                                                         cv_inputs,
                                                                                                         cv_outputs);

                _audio_frontend = std::make_unique<sushi::audio_frontend::OfflineFrontend>(_engine.get());
                break;
            }

        default:
            {
                return InitStatus::FAILED_AUDIO_FRONTEND_MISSING;
            }
    }

    auto audio_frontend_status = _audio_frontend->init(_frontend_config.get());
    if (audio_frontend_status != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        return InitStatus::FAILED_AUDIO_FRONTEND_INITIALIZATION;
    }

    return InitStatus::OK;
}

InitStatus FactoryBase::_set_up_control(const SushiOptions& options,
                                        sushi::jsonconfig::JsonConfigurator* configurator,
                                        int midi_inputs, int midi_outputs)
{
    _engine_controller = std::make_unique<sushi::engine::Controller>(_engine.get(),
                                                                     _midi_dispatcher.get(),
                                                                     _audio_frontend.get());

    if (options.frontend_type == FrontendType::JACK ||
        options.frontend_type == FrontendType::XENOMAI_RASPA ||
        options.frontend_type == FrontendType::PORTAUDIO)
    {
#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
        _midi_frontend = std::make_unique<sushi::midi_frontend::AlsaMidiFrontend>(midi_inputs,
                                                                                  midi_outputs,
                                                                                  _midi_dispatcher.get());
#elif SUSHI_BUILD_WITH_RT_MIDI
        _midi_frontend = std::make_unique<sushi::midi_frontend::RtMidiFrontend>(midi_inputs,
                                                                                midi_outputs,
                                                                                std::move(rt_midi_input_mappings),
                                                                                std::move(rt_midi_output_mappings),
                                                                                _midi_dispatcher.get());
#else
        _midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(midi_inputs,
                                                                                  midi_outputs,
                                                                                  _midi_dispatcher.get());
#endif

        auto oscpack_messenger = new sushi::osc::OscpackOscMessenger(options.osc_server_port,
                                                                     options.osc_send_port,
                                                                     options.osc_send_ip);

        _osc_frontend = std::make_unique<sushi::control_frontend::OSCFrontend>(_engine.get(),
                                                                               _engine_controller.get(),
                                                                               oscpack_messenger);

        _engine_controller->set_osc_frontend(_osc_frontend.get());

        auto osc_status = _osc_frontend->init();
        if (osc_status != sushi::control_frontend::ControlFrontendStatus::OK)
        {
            return InitStatus::FAILED_OSC_FRONTEND_INITIALIZATION;
        }

        if (configurator)
        {
            configurator->set_osc_frontend(_osc_frontend.get());

            auto status = configurator->load_osc();
            if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK &&
                status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
            {
                return InitStatus::FAILED_LOAD_OSC;
            }
        }
    }
    else if (options.frontend_type == FrontendType::PASSIVE)
    {
        _midi_frontend = std::make_unique<sushi::midi_frontend::PassiveMidiFrontend>(_midi_dispatcher.get());
    }
    else
    {
        _midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(_midi_dispatcher.get());
    }

    auto midi_ok = _midi_frontend->init();
    if (!midi_ok)
    {
        return InitStatus::FAILED_MIDI_FRONTEND_INITIALIZATION;
    }
    _midi_dispatcher->set_frontend(_midi_frontend.get());

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, _engine_controller.get());
#endif

    return InitStatus::OK;
}

} // namespace sushi