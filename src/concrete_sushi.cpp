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

#include "factories/passive_factory_implementation.h"

#include "concrete_sushi.h"

#include "engine/audio_engine.h"

#include "logging.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("concrete_sushi");

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

std::optional<std::pair<std::string, int>> SushiOptions::grpc_address_and_port()
{
    auto last_colon_index = grpc_listening_address.find_last_of(':');
    if (last_colon_index == std::string::npos)
    {
        return std::nullopt;
    }

    last_colon_index++; // to include the last ':' in the address part.

    auto address_part = grpc_listening_address.substr(0, last_colon_index);

    int port = -1;
    try
    {
        port = std::stoi(grpc_listening_address.substr(last_colon_index));
    }
    catch (...)
    {
        return std::nullopt;
    }

    return std::tie(address_part, port);
}

bool SushiOptions::increment_grpc_port_number()
{
    auto address_and_port = grpc_address_and_port();

    if (address_and_port.has_value())
    {
        grpc_listening_address = address_and_port->first +
                                 std::to_string(address_and_port->second + 1);
        return true;
    }
    else
    {
        return false;
    }
}

std::string to_string(Status status)
{
    switch (status)
    {
        case Status::FAILED_LOAD_HOST_CONFIG:
            return "Failed to load host configuration from config file.";
        case Status::FAILED_INVALID_CONFIGURATION_FILE:
            return "Error reading host config, check logs for details.";
        case Status::FAILED_LOAD_TRACKS:
            return "Failed to load tracks from the Json config file.";
        case Status::FAILED_LOAD_MIDI_MAPPING:
            return "Failed to load MIDI mapping from the Json config file.";
        case Status::FAILED_LOAD_CV_GATE:
            return "Failed to load CV and Gate configuration.";
        case Status::FAILED_LOAD_PROCESSOR_STATES:
            return "Failed to load the initial processor states.";
        case Status::FAILED_LOAD_EVENT_LIST:
            return "Failed to load Event list from the Json config file.";
        case Status::FAILED_LOAD_EVENTS:
            return "Failed to load Events from the Json config file.";
        case Status::FAILED_LOAD_OSC:
            return "Failed to load OSC echo specification from the Json config file.";
        case Status::FAILED_OSC_FRONTEND_INITIALIZATION:
            return "Failed to setup the OSC frontend.";
        case Status::FAILED_INVALID_FILE_PATH:
            return "Error reading config file, invalid file path: ";
        case Status::FAILED_XENOMAI_INITIALIZATION:
            return "Failed to initialize the Xenomai process, err. code: ";
        case Status::FAILED_AUDIO_FRONTEND_MISSING:
            return "No audio frontend is selected.";
        case Status::FAILED_AUDIO_FRONTEND_INITIALIZATION:
            return "Error initializing frontend, check logs for details.";
        case Status::FAILED_MIDI_FRONTEND_INITIALIZATION:
            return "Failed to setup the Midi frontend.";
        case Status::FAILED_TO_START_RPC_SERVER:
            return "Failed to start the RPC server.";
        case Status::OK:
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

ConcreteSushi::~ConcreteSushi()
{
    stop();
}

Status ConcreteSushi::start()
{
    if (_osc_frontend != nullptr)
    {
        _osc_frontend->run();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (_rpc_server != nullptr)
    {
        bool rpc_server_status = _rpc_server->start();
        if (!rpc_server_status)
        {
            if (_osc_frontend != nullptr)
            {
                _osc_frontend->stop();
            }
            return Status::FAILED_TO_START_RPC_SERVER;
        }
    }
#endif

    _audio_frontend->run();
    _engine->event_dispatcher()->run();
    _midi_frontend->run();

    return Status::OK;
}

void ConcreteSushi::stop()
{
    SUSHI_LOG_INFO("Stopping Sushi.");

    if (_audio_frontend != nullptr)
    {
        _audio_frontend->cleanup();
        _audio_frontend.reset();
    }

    if (_engine != nullptr)
    {
        _engine->event_dispatcher()->stop();
    }

    if (_osc_frontend != nullptr)
    {
        _osc_frontend->stop();
        _osc_frontend.reset();
    }

    if (_midi_frontend != nullptr)
    {
        _midi_frontend->stop();
        _midi_frontend.reset();
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    if (_rpc_server != nullptr)
    {
        _rpc_server->stop();
        _rpc_server.reset();
    }
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