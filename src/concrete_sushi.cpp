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

#include "concrete_sushi.h"

#include "engine/audio_engine.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "logging.h"

namespace sushi
{

void init_logger([[maybe_unused]] const SushiOptions& options)
{
    auto ret_code = SUSHI_INITIALIZE_LOGGER(options.log_filename,
                                            "Logger",
                                            options.log_level,
                                            options.enable_flush_interval,
                                            options.log_flush_interval,
                                            options.sentry_crash_handler_path,
                                            options.sentry_dsn);

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

ConcreteSushi::ConcreteSushi() = default;
ConcreteSushi::~ConcreteSushi() = default;

void ConcreteSushi::start()
{
    _audio_frontend->run();
    _engine->event_dispatcher()->run();
    _midi_frontend->run();

    if (_osc_frontend != nullptr)
    {
        _osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (_rpc_server != nullptr)
    {
        _rpc_server->start();
    }
#endif
}

void ConcreteSushi::exit()
{
    _audio_frontend->cleanup();
    _engine->event_dispatcher()->stop();

    if (_osc_frontend != nullptr)
    {
        _osc_frontend->stop();
    }

    _midi_frontend->stop();

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server->stop();
#endif
}

ext::SushiControl* ConcreteSushi::controller()
{
    return _engine_controller.get();
}

void ConcreteSushi::set_sample_rate(float sample_rate)
{
    _engine->set_sample_rate(sample_rate);
}

float ConcreteSushi::sample_rate() const
{
    return _engine->sample_rate();
}

} // namespace Sushi