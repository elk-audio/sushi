/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "audio_routing_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

std::vector<ext::AudioConnection> AudioRoutingController::get_all_input_connections() const
{
    return std::vector<ext::AudioConnection>();
}

std::vector<ext::AudioConnection> AudioRoutingController::get_all_output_connections() const
{
    return std::vector<ext::AudioConnection>();
}

std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>> AudioRoutingController::get_input_connections_for_track(int track_id) const
{
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, std::vector<ext::AudioConnection>()};
}

std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>> AudioRoutingController::get_output_connections_for_track(int track_id) const
{
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, std::vector<ext::AudioConnection>()};
}

ext::ControlStatus AudioRoutingController::connect_input_channel_to_track(int track_id, int track_channel, int input_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::connect_output_channel_to_track(int track_id, int track_channel, int output_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_input(int track_id, int track_channel, int input_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_output(int track_id, int track_channel, int output_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_all_inputs_from_track(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_all_outputs_from_track(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi
