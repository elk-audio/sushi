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

#include "system_controller.h"

namespace sushi {
namespace engine {
namespace controller_impl {


std::string SystemController::get_interface_version() const
{
    return "";
}

std::string SystemController::get_sushi_version() const
{
    return "";
}

ext::SushiBuildInfo SystemController::get_sushi_build_info() const
{
    return ext::SushiBuildInfo();
}

int SystemController::get_input_audio_channel_count() const
{
    return 0;
}

int SystemController::get_output_audio_channel_count() const
{
    return 0;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi