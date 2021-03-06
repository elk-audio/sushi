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

#include "parameter_controller.h"
#include "engine/base_engine.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

inline ext::ParameterType to_external(const sushi::ParameterType type)
{
    switch (type)
    {
        case ParameterType::FLOAT:      return ext::ParameterType::FLOAT;
        case ParameterType::INT:        return ext::ParameterType::INT;
        case ParameterType::BOOL:       return ext::ParameterType::BOOL;
        case ParameterType::STRING:     return ext::ParameterType::STRING_PROPERTY;
        case ParameterType::DATA:       return ext::ParameterType::DATA_PROPERTY;
        default:                        return ext::ParameterType::FLOAT;
    }
}

inline std::vector<ext::ParameterInfo>  _read_parameters(const Processor* processor)
{
    assert(processor != nullptr);
    std::vector<ext::ParameterInfo> infos;
    const auto& params = processor->all_parameters();
    for (const auto& param : params)
    {
        ext::ParameterInfo info;
        info.id = param->id();
        info.type = ext::ParameterType::FLOAT;
        info.label = param->label();
        info.name = param->name();
        info.unit = param->unit();
        info.automatable = param->automatable();
        info.min_domain_value = param->min_domain_value();
        info.max_domain_value = param->max_domain_value();
        infos.push_back(info);
    }
    return infos;
}

ParameterController::ParameterController(BaseEngine* engine) : _engine(engine),
                                                               _event_dispatcher(engine->event_dispatcher()),
                                                               _processors(engine->processor_container())
{}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> ParameterController::get_processor_parameters(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_parameters called with processor {}", processor_id);
    const auto proc = _processors->processor(processor_id);
    if (proc)
    {
        return {ext::ControlStatus::OK, _read_parameters(proc.get())};
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> ParameterController::get_track_parameters(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_parameters called with processor {}", track_id);
    const auto track = _processors->track(track_id);
    if (track)
    {
        return {ext::ControlStatus::OK, _read_parameters(track.get())};
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> ParameterController::get_parameter_id(int processor_id, const std::string& parameter_name) const
{
    SUSHI_LOG_DEBUG("get_parameter_id called with processor {} and parameter {}", processor_id, parameter_name);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, 0};
    }
    auto descr = processor->parameter_from_name(parameter_name);
    if (descr != nullptr)
    {
        return {ext::ControlStatus::OK, descr->id()};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::ParameterInfo> ParameterController::get_parameter_info(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_info called with processor {} and parameter {}", processor_id, parameter_id);
    ext::ParameterInfo info;
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto descr = processor->parameter_from_id(static_cast<ObjectId>(parameter_id));
        if (descr != nullptr)
        {
            info.id = descr->id();
            info.label = descr->label();
            info.name = descr->name();
            info.unit = descr->unit();
            info.type = to_external(descr->type());
            info.min_domain_value = descr->min_domain_value();
            info.max_domain_value = descr->max_domain_value();
            info.automatable =  descr->type() == ParameterType::FLOAT || // TODO - this might not be the way we eventually want it
                                descr->type() == ParameterType::INT   ||
                                descr->type() == ParameterType::BOOL;
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, float> ParameterController::get_parameter_value(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_value called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, float> ParameterController::get_parameter_value_in_domain(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_value_normalised called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_in_domain(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, std::string> ParameterController::get_parameter_value_as_string(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_value_as_string called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_formatted(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, std::string> ParameterController::get_string_property_value(int /*processor_id*/, int /*parameter_id*/) const
{
    SUSHI_LOG_DEBUG("get_string_property_value called");
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, ""};
}

ext::ControlStatus ParameterController::set_parameter_value(int processor_id, int parameter_id, float value)
{
    float clamped_value = std::clamp<float>(value, 0.0f, 1.0f);
    SUSHI_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id, clamped_value);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          clamped_value,
                                          IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus ParameterController::set_string_property_value(int /*processor_id*/, int /*parameter_id*/, const std::string& /*value*/)
{
    SUSHI_LOG_DEBUG("set_string_property_value called");
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi
