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
 * @brief Implementation of external control interface of sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "osc_controller.h"

namespace sushi {
namespace engine {
namespace controller_impl {

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"


// TODO Ilias: Q Where do we keep these values now?

int OscController::get_send_port() const
{
    return 0;
}

int OscController::get_receive_port() const
{
    return 0;
}

std::vector<std::string> OscController::get_enabled_parameter_outputs() const
{
    return std::vector<std::string>();
}

ext::ControlStatus OscController::enable_output_for_parameter(int processor_id, int parameter_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus OscController::disable_output_for_parameter(int processor_id, int parameter_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

void OscController::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi