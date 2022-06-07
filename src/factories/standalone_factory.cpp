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

#include "standalone_factory.h"

#include "logging.h"

#include "engine/audio_engine.h"
#include "src/sushi.h"

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RT_MIDI
#include "control_frontends/rt_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/portaudio_frontend.h"
#include "audio_frontends/xenomai_raspa_frontend.h"

#include "engine/json_configurator.h"
#include "control_frontends/oscpack_osc_messenger.h"

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi-factory");

StandaloneFactory::StandaloneFactory() = default;

StandaloneFactory::~StandaloneFactory() = default;

std::unique_ptr<AbstractSushi> StandaloneFactory::run(SushiOptions& options)
{
    init_logger(options);

    // TODO: TEST THAT THIS WORKS!

#ifdef SUSHI_BUILD_WITH_XENOMAI
    auto raspa_status = audio_frontend::XenomaiRaspaFrontend::global_init();
    if (raspa_status < 0)
    {
        _status = INIT_STATUS::FAILED_XENOMAI_INITIALIZATION;
        return nullptr;
    }

    if (options.frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }
#endif

    _instantiate_subsystems(options);

    return _make_sushi();
}

InitStatus StandaloneFactory::_setup_audio_frontend(const SushiOptions& options,
                                                    const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    switch (options.frontend_type)
    {
        case FrontendType::JACK:
        {
            SUSHI_LOG_INFO("Setting up Jack audio frontend");
            _frontend_config = std::make_unique<audio_frontend::JackFrontendConfiguration>(options.jack_client_name,
                                                                                           options.jack_server_name,
                                                                                           options.connect_ports,
                                                                                           cv_inputs,
                                                                                           cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::JackFrontend>(_engine.get());
            break;
        }
        case FrontendType::PORTAUDIO:
        {
            SUSHI_LOG_INFO("Setting up PortAudio frontend");
            _frontend_config = std::make_unique<audio_frontend::PortAudioFrontendConfiguration>(options.portaudio_input_device_id,
                                                                                                options.portaudio_output_device_id,
                                                                                                cv_inputs,
                                                                                                cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::PortAudioFrontend>(_engine.get());
            break;
        }
        case FrontendType::XENOMAI_RASPA:
        {
            SUSHI_LOG_INFO("Setting up Xenomai RASPA frontend");
            _frontend_config = std::make_unique<audio_frontend::XenomaiRaspaFrontendConfiguration>(options.debug_mode_switches,
                                                                                                   cv_inputs,
                                                                                                   cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::XenomaiRaspaFrontend>(_engine.get());
            break;
        }
        default:
        {
            return InitStatus::FAILED_AUDIO_FRONTEND_MISSING;
        }
    }

    return InitStatus::OK;
}

InitStatus StandaloneFactory::_set_up_midi([[maybe_unused]] const SushiOptions& options, const jsonconfig::ControlConfig& config)
{
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
    _midi_frontend = std::make_unique<midi_frontend::AlsaMidiFrontend>(midi_inputs,
                                                                       midi_outputs,
                                                                       _midi_dispatcher.get());
#elif SUSHI_BUILD_WITH_RT_MIDI
    auto rt_midi_input_mappings = config.rt_midi_input_mappings;
    auto rt_midi_output_mappings = config.rt_midi_output_mappings;

    _midi_frontend = std::make_unique<midi_frontend::RtMidiFrontend>(midi_inputs,
                                                                     midi_outputs,
                                                                     std::move(rt_midi_input_mappings),
                                                                     std::move(rt_midi_output_mappings),
                                                                     _midi_dispatcher.get());
#else
    _midi_frontend = std::make_unique<midi_frontend::NullMidiFrontend>(midi_inputs,
                                                                       midi_outputs,
                                                                       _midi_dispatcher.get());
#endif

    return InitStatus::OK;
}

InitStatus StandaloneFactory::_set_up_control(const SushiOptions& options, jsonconfig::JsonConfigurator* configurator)
{
    _engine_controller = std::make_unique<engine::Controller>(_engine.get(),
                                                              _midi_dispatcher.get(),
                                                              _audio_frontend.get());

    auto oscpack_messenger = new osc::OscpackOscMessenger(options.osc_server_port,
                                                          options.osc_send_port,
                                                          options.osc_send_ip);

    _osc_frontend = std::make_unique<control_frontend::OSCFrontend>(_engine.get(),
                                                                    _engine_controller.get(),
                                                                    oscpack_messenger);

    _engine_controller->set_osc_frontend(_osc_frontend.get());

    auto osc_status = _osc_frontend->init();
    if (osc_status != control_frontend::ControlFrontendStatus::OK)
    {
        return InitStatus::FAILED_OSC_FRONTEND_INITIALIZATION;
    }

    if (configurator)
    {
        configurator->set_osc_frontend(_osc_frontend.get());

        auto status = configurator->load_osc();
        if (status != jsonconfig::JsonConfigReturnStatus::OK &&
            status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return InitStatus::FAILED_LOAD_OSC;
        }
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, _engine_controller.get());
    SUSHI_LOG_INFO("Instantiating gRPC server with address: {}", options.grpc_listening_address);
#endif

    return InitStatus::OK;
}

InitStatus StandaloneFactory::_load_json_events([[maybe_unused]] const SushiOptions& options, jsonconfig::JsonConfigurator* configurator)
{
    auto status = configurator->load_events();

    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_EVENTS;
    }

    return InitStatus::OK;
}

} // namespace sushi