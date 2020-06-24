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
 * @brief Internal implementation details common to all sub-controllers
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONTROLLER_INTERNAL_H
#define SUSHI_CONTROLLER_INTERNAL_H

#include "engine/base_engine.h"
#include "include/control_interface.h"

namespace sushi {
namespace engine {
namespace controller_impl {

/**
 * @brief Default error mapping. Some controllers might want to override this.
 */
ext::ControlStatus default_error_mapping(engine::EngineReturnStatus status)
{
    switch (status)
    {
        case EngineReturnStatus::OK:                return ext::ControlStatus::OK;
        case EngineReturnStatus::ERROR:             return ext::ControlStatus::ERROR;

        case EngineReturnStatus::INVALID_PLUGIN:
        case EngineReturnStatus::INVALID_PROCESSOR:
        case EngineReturnStatus::INVALID_PARAMETER:
        case EngineReturnStatus::INVALID_TRACK:     return ext::ControlStatus::NOT_FOUND;

        case EngineReturnStatus::INVALID_PLUGIN_UID:
        case EngineReturnStatus::INVALID_N_CHANNELS:
        case EngineReturnStatus::INVALID_PLUGIN_TYPE:
        case EngineReturnStatus::INVALID_BUS:       return ext::ControlStatus::INVALID_ARGUMENTS;

        default:                                    return ext::ControlStatus::ERROR;
    }
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_CONTROLLER_INTERNAL_H
