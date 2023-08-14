/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Implementation of external control interface for sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"

#include "audio_routing_controller.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi::internal::engine::controller_impl {

inline control::AudioConnection to_external(const AudioConnection& con)
{
    return control::AudioConnection{.track_id = static_cast<int>(con.track),
                                .track_channel = con.track_channel,
                                .engine_channel = con.engine_channel};
}


std::vector<control::AudioConnection> AudioRoutingController::get_all_input_connections() const
{
    ELKLOG_LOG_DEBUG("get_all_input_connections called");
    auto connections = _engine->audio_input_connections();
    std::vector<control::AudioConnection> returns;
    returns.reserve(connections.size());
    for (const auto& connection : connections)
    {
        returns.push_back(to_external(connection));
    }
    return returns;
}

std::vector<control::AudioConnection> AudioRoutingController::get_all_output_connections() const
{
    ELKLOG_LOG_DEBUG("get_all_output_connections called");
    auto connections = _engine->audio_output_connections();
    std::vector<control::AudioConnection> returns;
    returns.reserve(connections.size());
    for (const auto& connection : connections)
    {
        returns.push_back(to_external(connection));
    }
    return returns;
}

std::vector<control::AudioConnection> AudioRoutingController::get_input_connections_for_track(int track_id) const
{
    ELKLOG_LOG_DEBUG("get_input_connections_for_track called with track id {}", track_id);
    auto connections = _engine->audio_input_connections();
    std::vector<control::AudioConnection> returns;
    for (const auto& connection : connections)
    {
        if (connection.track == static_cast<ObjectId>(track_id))
        {
            returns.push_back(to_external(connection));
        }
    }
    return returns;
}

std::vector<control::AudioConnection> AudioRoutingController::get_output_connections_for_track(int track_id) const
{
    ELKLOG_LOG_DEBUG("get_output_connections_for_track called with track id {}", track_id);
    auto connections = _engine->audio_output_connections();
    std::vector<control::AudioConnection> returns;
    for (const auto& connection : connections)
    {
        if (connection.track == static_cast<ObjectId>(track_id))
        {
            returns.push_back(to_external(connection));
        }
    }
    return returns;
}

control::ControlStatus AudioRoutingController::connect_input_channel_to_track(int track_id, int track_channel, int input_channel)
{
    ELKLOG_LOG_DEBUG("disconnect_output called with track id {}, track_channel {}, input_channel {}", track_id, track_channel, input_channel);
    auto lambda = [=] () -> int
    {
        auto status = _engine->connect_audio_input_channel(input_channel, track_channel, track_id);
        ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Connecting audio channel {} to channel {} of track id {} failed with error {}",
                input_channel, track_channel, track_id, status)

        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus AudioRoutingController::connect_output_channel_to_track(int track_id, int track_channel, int output_channel)
{
    ELKLOG_LOG_DEBUG("connect_output called with track id {}, track_channel {}, output_channel {}", track_id, track_channel, output_channel);
    auto lambda = [=] () -> int
    {
        auto status = _engine->connect_audio_output_channel(output_channel, track_channel, track_id);
        ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Connecting audio channel {} from channel {} of track id {} failed with error {}",
                           output_channel, track_channel, track_id, status)

        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus AudioRoutingController::disconnect_input(int track_id, int track_channel, int input_channel)
{
    ELKLOG_LOG_DEBUG("disconnect_input called with track id {}, track_channel {}, input_channel {}", track_id, track_channel, input_channel);
    auto lambda = [=] () -> int
    {
        auto status = _engine->disconnect_audio_input_channel(input_channel, track_channel, track_id);
        ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Disconnecting audio channel {} to channel {} of track id {} failed with error {}",
                           input_channel, track_channel, track_id, status)

        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus AudioRoutingController::disconnect_output(int track_id, int track_channel, int output_channel)
{
    ELKLOG_LOG_DEBUG("disconnect_output called with track id {}, track_channel {}, output_channel {}", track_id, track_channel, output_channel);
    auto lambda = [=] () -> int
    {
        auto status = _engine->disconnect_audio_output_channel(output_channel, track_channel, track_id);
        ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Disconnecting audio channel {} from channel {} of track id {} failed with error {}",
                           output_channel, track_channel, track_id, status)

        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus AudioRoutingController::disconnect_all_inputs_from_track(int track_id)
{
    ELKLOG_LOG_DEBUG("disconnect_all_inputs_from_track called with track {}", track_id);
    auto lambda = [=] () -> int
    {
        auto connections = _engine->audio_input_connections();
        int return_status = EventStatus::HANDLED_OK;
        for (const auto& connection : connections)
        {
            if (connection.track == static_cast<ObjectId>(track_id))
            {
                auto status = _engine->disconnect_audio_input_channel(connection.engine_channel,
                                                                      connection.track_channel,
                                                                      connection.track);
                ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Disconnecting audio channel {} from channel {} of track id {} failed with error {}",
                                   connection.engine_channel, connection.track_channel, connection.track, status)

                return_status = status == EngineReturnStatus::OK ? return_status : EventStatus::ERROR;
            }
        }
        return return_status;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus AudioRoutingController::disconnect_all_outputs_from_track(int track_id)
{
    ELKLOG_LOG_DEBUG("disconnect_all_outputs_from_track called with track {}", track_id);
    auto lambda = [=] () -> int
    {
        auto connections = _engine->audio_output_connections();
        int return_status = EventStatus::HANDLED_OK;
        for (const auto& connection : connections)
        {
            if (connection.track == static_cast<ObjectId>(track_id))
            {
                auto status = _engine->disconnect_audio_output_channel(connection.engine_channel,
                                                                       connection.track_channel,
                                                                       connection.track);
                ELKLOG_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Disconnecting audio channel {} from channel {} of track id {} failed with error {}",
                                   connection.engine_channel, connection.track_channel, connection.track, status)
                return_status = status == EngineReturnStatus::OK ? return_status : EventStatus::ERROR;
            }
        }
        return return_status;
    };
    
    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

} // end namespace sushi::engine::controller_impl