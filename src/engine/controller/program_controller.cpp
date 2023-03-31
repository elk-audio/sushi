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

#include "program_controller.h"

#include "sushi/logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi::internal::engine::controller_impl {

ProgramController::ProgramController(BaseEngine* engine) : /* _engine(engine), */
                                                           _event_dispatcher(engine->event_dispatcher()),
                                                           _processors(engine->processor_container())
{}

std::pair<control::ControlStatus, int> ProgramController::get_processor_current_program(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_current_program called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, 0};
    }
    if (processor->supports_programs())
    {
        return {control::ControlStatus::OK, processor->current_program()};
    }
    return {control::ControlStatus::UNSUPPORTED_OPERATION, 0};
}

std::pair<control::ControlStatus, std::string> ProgramController::get_processor_current_program_name(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_current_program_name called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, ""};
    }
    if (processor->supports_programs())
    {
        return {control::ControlStatus::OK, processor->current_program_name()};
    }
    return {control::ControlStatus::UNSUPPORTED_OPERATION, ""};
}

std::pair<control::ControlStatus, std::string> ProgramController::get_processor_program_name(int processor_id, int program_id) const
{
    SUSHI_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, ""};
    }
    else if (processor->supports_programs() == false)
    {
        return {control::ControlStatus::UNSUPPORTED_OPERATION, ""};
    }
    auto [status, name] = processor->program_name(program_id);
    if (status == ProcessorReturnCode::OK)
    {
        return {control::ControlStatus::OK, std::move(name)};
    }
    return {control::ControlStatus::OUT_OF_RANGE, ""};
}

std::pair<control::ControlStatus, std::vector<std::string>> ProgramController::get_processor_programs(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, std::vector<std::string>()};
    }
    else if (processor->supports_programs() == false)
    {
        return {control::ControlStatus::UNSUPPORTED_OPERATION, std::vector<std::string>()};
    }
    auto [status, names] = processor->all_program_names();
    if (status == ProcessorReturnCode::OK)
    {
        return {control::ControlStatus::OK, std::move(names)};
    }
    return {control::ControlStatus::OUT_OF_RANGE, std::vector<std::string>()};
}

control::ControlStatus ProgramController::set_processor_program(int processor_id, int program_id)
{
    SUSHI_LOG_DEBUG("set_processor_program called with processor {} and program {}", processor_id, program_id);
    auto event = new ProgramChangeEvent(static_cast<ObjectId>(processor_id), program_id, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return control::ControlStatus::OK;
}

} // end namespace sushi::internal::engine::controller_impl
