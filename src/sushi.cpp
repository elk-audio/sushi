/*
* Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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

#include "logging.h"

#include "sushi.h"

#include "engine/audio_engine.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi");

void init_logger(const SushiOptions& options)
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

std::string to_string(InitStatus init_status)
{
    switch (init_status)
    {
        case InitStatus::FAILED_LOAD_HOST_CONFIG:
            return "Failed to load host configuration from config file";
        case InitStatus::FAILED_INVALID_CONFIGURATION_FILE:
            return "Error reading host config, check logs for details.";
        case InitStatus::FAILED_LOAD_TRACKS:
            return "Failed to load tracks from Json config file";
        case InitStatus::FAILED_LOAD_MIDI_MAPPING:
            return "Failed to load MIDI mapping from Json config file";
        case InitStatus::FAILED_LOAD_CV_GATE:
            return "Failed to load CV and Gate configuration";
        case InitStatus::FAILED_LOAD_PROCESSOR_STATES:
            return "Failed to load initial processor states";
        case InitStatus::FAILED_LOAD_EVENT_LIST:
            return "Failed to load Event list from Json config file";
        case InitStatus::FAILED_LOAD_EVENTS:
            return "Failed to load Events from Json config file";
        case InitStatus::FAILED_LOAD_OSC:
            return "Failed to load OSC echo specification from Json config file";
        case InitStatus::FAILED_OSC_FRONTEND_INITIALIZATION:
            return "Failed to setup OSC frontend";
        case InitStatus::FAILED_INVALID_FILE_PATH:
            return "Error reading config file, invalid file path: ";
        case InitStatus::FAILED_XENOMAI_INITIALIZATION:
            return "Failed to initialize Xenomai process, err. code: ";
        case InitStatus::FAILED_AUDIO_FRONTEND_MISSING:
            return "No audio frontend selected";
        case InitStatus::FAILED_AUDIO_FRONTEND_INITIALIZATION:
            return "Error initializing frontend, check logs for details.";
        case InitStatus::FAILED_MIDI_FRONTEND_INITIALIZATION:
            return "Failed to setup Midi frontend";
        case InitStatus::OK:
            return "Ok";
        default:
            assert(false);
            return "";
    }
}

///////////////////////////////////////////
// Sushi methods                         //
///////////////////////////////////////////

Sushi::Sushi(const sushi::SushiOptions& options,
             std::unique_ptr<engine::AudioEngine> engine,
             std::unique_ptr<midi_dispatcher::MidiDispatcher> midi_dispatcher,
             std::unique_ptr<midi_frontend::BaseMidiFrontend> midi_frontend,
             std::unique_ptr<control_frontend::OSCFrontend> osc_frontend,
             std::unique_ptr<audio_frontend::BaseAudioFrontend> audio_frontend,
             std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> frontend_config,
             std::unique_ptr<engine::Controller> engine_controller,
             std::unique_ptr<sushi_rpc::GrpcServer> rpc_server) : _options(options),
                                                                  _engine(std::move(engine)),
                                                                  _midi_dispatcher(std::move(midi_dispatcher)),
                                                                  _midi_frontend(std::move(midi_frontend)),
                                                                  _osc_frontend(std::move(osc_frontend)),
                                                                  _audio_frontend(std::move(audio_frontend)),
                                                                  _frontend_config(std::move(frontend_config)),
                                                                  _engine_controller(std::move(engine_controller)),
                                                                  _rpc_server(std::move(rpc_server))
{

}

Sushi::~Sushi() = default;

void Sushi::start()
{
    if (_options.enable_timings)
    {
        _engine->performance_timer()->enable(true);
    }

    _audio_frontend->run();
    _engine->event_dispatcher()->run();
    _midi_frontend->run();

    if (_options.frontend_type == FrontendType::JACK ||
        _options.frontend_type == FrontendType::XENOMAI_RASPA ||
        _options.frontend_type == FrontendType::PORTAUDIO)
    {
        _osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    SUSHI_LOG_INFO("Starting gRPC server with address: {}", _options.grpc_listening_address);
    _rpc_server->start();
#endif
}

void Sushi::exit()
{
    _audio_frontend->cleanup();
    _engine->event_dispatcher()->stop();

    if (_options.frontend_type == FrontendType::JACK ||
        _options.frontend_type == FrontendType::XENOMAI_RASPA ||
        _options.frontend_type == FrontendType::PORTAUDIO)
    {
        _osc_frontend->stop();
        _midi_frontend->stop();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server->stop();
#endif
}

engine::Controller* Sushi::controller()
{
    return _engine_controller.get();
}

engine::Controller* Sushi::controller()
{
    return _controller.get();
}

void Sushi::set_sample_rate(float sample_rate)
{
    _engine->set_sample_rate(sample_rate);
}

float Sushi::sample_rate() const
{
    return _engine->sample_rate();
}


} // namespace Sushi