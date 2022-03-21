/*
* Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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

/**
* @brief Main entry point to Sushi
* @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#include <iostream>

#include "twine/src/twine_internal.h"
#include "logging.h"

#include "sushi.h"

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi");

std::string to_string(INIT_STATUS init_status)
{
    switch (init_status)
    {
        case INIT_STATUS::FAILED_LOAD_HOST_CONFIG:
            return "Failed to load host configuration from config file";
        case INIT_STATUS::FAILED_INVALID_CONFIGURATION_FILE:
            return "Error reading host config, check logs for details.";
        case INIT_STATUS::FAILED_LOAD_TRACKS:
            return "Failed to load tracks from Json config file";
        case INIT_STATUS::FAILED_LOAD_MIDI_MAPPING:
            return "Failed to load MIDI mapping from Json config file";
        case INIT_STATUS::FAILED_LOAD_CV_GATE:
            return "Failed to load CV and Gate configuration";
        case INIT_STATUS::FAILED_LOAD_PROCESSOR_STATES:
            return "Failed to load initial processor states";
        case INIT_STATUS::FAILED_LOAD_EVENT_LIST:
            return "Failed to load Event list from Json config file";
        case INIT_STATUS::FAILED_LOAD_EVENTS:
            return "Failed to load Events from Json config file";
        case INIT_STATUS::FAILED_LOAD_OSC:
            return "Failed to load OSC echo specification from Json config file";
        case INIT_STATUS::FAILED_OSC_FRONTEND_INITIALIZATION:
            return "Failed to setup OSC frontend";
        case INIT_STATUS::FAILED_INVALID_FILE_PATH:
            return "Error reading config file, invalid file path: ";
        case INIT_STATUS::FAILED_TWINE_INITIALIZATION:
            return "Failed to initialize Xenomai process, err. code: ";
        case INIT_STATUS::FAILED_AUDIO_FRONTEND_MISSING:
            return "No audio frontend selected";
        case INIT_STATUS::FAILED_AUDIO_FRONTEND_INITIALIZATION:
            return "Error initializing frontend, check logs for details.";
        case INIT_STATUS::FAILED_MIDI_FRONTEND_INITIALIZATION:
            return "Failed to setup Midi frontend";
        case INIT_STATUS::OK:
            return "Ok";
    };

    assert(false);
    return "";
}

sushi::INIT_STATUS sushi::Sushi::init(sushi::SushiOptions& options)
{
    _init_logger(options);

#ifdef SUSHI_BUILD_WITH_XENOMAI
    _raspa_status = sushi::audio_frontend::XenomaiRaspaFrontend::global_init();
    if (_raspa_status < 0)
    {
        return INIT_STATUS::FAILED_TWINE_INITIALIZATION;
    }
#endif

    // TODO: Why is this outside of the above ifdef?
    if (options.frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }

    _engine = std::make_unique<engine::AudioEngine>(CompileTimeSettings::sample_rate_default,
                                                    options.rt_cpu_cores,
                                                    options.debug_mode_switches,
                                                    nullptr);

    _midi_dispatcher = std::make_unique<sushi::midi_dispatcher::MidiDispatcher>(_engine->event_dispatcher());

    _configurator = std::make_unique<sushi::jsonconfig::JsonConfigurator>(_engine.get(),
                                                                          _midi_dispatcher.get(),
                                                                          _engine->processor_container(),
                                                                          options.config_filename);

    auto [audio_config_status, audio_config] = _configurator->load_audio_config();
    if (audio_config_status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        if (audio_config_status == sushi::jsonconfig::JsonConfigReturnStatus::INVALID_FILE)
        {
            return INIT_STATUS::FAILED_INVALID_FILE_PATH;
        }

        return INIT_STATUS::FAILED_INVALID_CONFIGURATION_FILE;
    }

    int midi_inputs = audio_config.midi_inputs.value_or(1);
    int midi_outputs = audio_config.midi_outputs.value_or(1);

#ifdef SUSHI_BUILD_WITH_RT_MIDI
    auto rt_midi_input_mappings = audio_config.rt_midi_input_mappings;
    auto rt_midi_output_mappings = audio_config.rt_midi_output_mappings;
#endif

    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Audio Frontend //
    ////////////////////////////////////////////////////////////////////////////////
    int cv_inputs = audio_config.cv_inputs.value_or(0);
    int cv_outputs = audio_config.cv_outputs.value_or(0);

    auto audio_frontend_status = _setup_audio_frontend(options, cv_inputs, cv_outputs);
    if (audio_frontend_status != INIT_STATUS::OK)
    {
        return audio_frontend_status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Load Configuration //
    ////////////////////////////////////////////////////////////////////////////////
    auto configuration_status = _load_configuration(options);
    if (configuration_status != INIT_STATUS::OK)
    {
        return configuration_status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Set up Controller and Control Frontends //
    ////////////////////////////////////////////////////////////////////////////////
    auto control_status = _set_up_control(options, midi_inputs, midi_outputs);
    if (control_status != INIT_STATUS::OK)
    {
        return control_status;
    }

    _configurator.reset();

    return INIT_STATUS::OK;
}

void Sushi::start(SushiOptions& options)
{
    if (options.enable_timings)
    {
        _engine->performance_timer()->enable(true);
    }

    _audio_frontend->run();
    _engine->event_dispatcher()->run();
    _midi_frontend->run();

    if (options.frontend_type == FrontendType::JACK ||
        options.frontend_type == FrontendType::XENOMAI_RASPA ||
        options.frontend_type == FrontendType::PORTAUDIO)
    {
        _osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    SUSHI_LOG_INFO("Starting gRPC server with address: {}", options.grpc_listening_address);
    _rpc_server->start();
#endif
}

void Sushi::exit(SushiOptions& options)
{
    _audio_frontend->cleanup();
    _engine->event_dispatcher()->stop();

    if (options.frontend_type == FrontendType::JACK ||
        options.frontend_type == FrontendType::XENOMAI_RASPA ||
        options.frontend_type == FrontendType::PORTAUDIO)
    {
        _osc_frontend->stop();
        _midi_frontend->stop();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server->stop();
#endif
}

void Sushi::_init_logger(SushiOptions& options)
{
    auto ret_code = SUSHI_INITIALIZE_LOGGER(options.log_filename,
                                            "Logger",
                                            options.log_level,
                                            options.enable_flush_interval,
                                            options.log_flush_interval);

    if (ret_code != SUSHI_LOG_ERROR_CODE_OK)
    {
        std::cerr << SUSHI_LOG_GET_ERROR_MESSAGE(ret_code) << ", using default." << std::endl;
    }
}

sushi::INIT_STATUS sushi::Sushi::_load_configuration(sushi::SushiOptions& options)
{
    auto status = _configurator->load_host_config();
    if(status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return INIT_STATUS::FAILED_LOAD_HOST_CONFIG;
    }

    status = _configurator->load_tracks();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK)
    {
        return INIT_STATUS::FAILED_LOAD_TRACKS;
    }

    status = _configurator->load_midi();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_MIDI_MAPPING;
    }

    status = _configurator->load_cv_gate();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_CV_GATE;
    }

    status = _configurator->load_initial_state();
    if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK && status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return INIT_STATUS::FAILED_LOAD_PROCESSOR_STATES;
    }

    if (options.frontend_type == FrontendType::DUMMY ||
        options.frontend_type == FrontendType::OFFLINE)
    {
        auto [status, events] = _configurator->load_event_list();
        if(status == jsonconfig::JsonConfigReturnStatus::OK)
        {
            static_cast<sushi::audio_frontend::OfflineFrontend*>(_audio_frontend.get())->add_sequencer_events(events);
        }
        else if (status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return INIT_STATUS::FAILED_LOAD_EVENT_LIST;
        }
    }
    else
    {
        status = _configurator->load_events();
        if (status != jsonconfig::JsonConfigReturnStatus::OK && status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return INIT_STATUS::FAILED_LOAD_EVENTS;
        }
    }

    return INIT_STATUS::OK;
}

INIT_STATUS Sushi::_setup_audio_frontend(SushiOptions& options, int cv_inputs, int cv_outputs)
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

        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
        {
            bool dummy = false;
            if (options.frontend_type == FrontendType::DUMMY)
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
            return INIT_STATUS::FAILED_AUDIO_FRONTEND_MISSING;
        }
    }

    auto audio_frontend_status = _audio_frontend->init(_frontend_config.get());
    if (audio_frontend_status != sushi::audio_frontend::AudioFrontendStatus::OK)
    {
        return INIT_STATUS::FAILED_AUDIO_FRONTEND_INITIALIZATION;
    }

    return INIT_STATUS::OK;
}

INIT_STATUS Sushi::_set_up_control(SushiOptions& options, int midi_inputs, int midi_outputs)
{
    _controller = std::make_unique<sushi::engine::Controller>(_engine.get(), _midi_dispatcher.get());

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

        _osc_frontend = std::make_unique<sushi::control_frontend::OSCFrontend>(_engine.get(),
                                                                               _controller.get(),
                                                                               options.osc_server_port,
                                                                               options.osc_send_port);

        _controller->set_osc_frontend(_osc_frontend.get());
        _configurator->set_osc_frontend(_osc_frontend.get());

        auto osc_status = _osc_frontend->init();
        if (osc_status != sushi::control_frontend::ControlFrontendStatus::OK)
        {
            return INIT_STATUS::FAILED_OSC_FRONTEND_INITIALIZATION;
        }

        auto status = _configurator->load_osc();
        if (status != sushi::jsonconfig::JsonConfigReturnStatus::OK &&
            status != sushi::jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
        {
            return INIT_STATUS::FAILED_LOAD_OSC;
        }
    }
    else
    {
        _midi_frontend = std::make_unique<sushi::midi_frontend::NullMidiFrontend>(_midi_dispatcher.get());
    }

    auto midi_ok = _midi_frontend->init();
    if (!midi_ok)
    {
        return INIT_STATUS::FAILED_MIDI_FRONTEND_INITIALIZATION;
    }
    _midi_dispatcher->set_frontend(_midi_frontend.get());

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, _controller.get());
#endif

    return INIT_STATUS::OK;
}

} // namespace Sushi