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

#include "parameter_controller.h"

#include "engine/base_engine.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi::internal::engine::controller_impl {

inline control::ParameterType to_external(const ParameterType type)
{
    switch (type)
    {
        case ParameterType::FLOAT:      return control::ParameterType::FLOAT;
        case ParameterType::INT:        return control::ParameterType::INT;
        case ParameterType::BOOL:       return control::ParameterType::BOOL;
        default:                        return control::ParameterType::FLOAT;
    }
}

inline std::vector<control::ParameterInfo> _read_parameters(const Processor* processor)
{
    assert(processor != nullptr);
    std::vector<control::ParameterInfo> infos;
    const auto& params = processor->all_parameters();
    for (const auto& param : params)
    {
        if (param->type() == ParameterType::FLOAT || param->type() == ParameterType::INT || param->type() == ParameterType::BOOL)
        {
            control::ParameterInfo info;
            info.id = param->id();
            info.type = to_external(param->type());
            info.label = param->label();
            info.name = param->name();
            info.unit = param->unit();
            info.automatable = param->automatable();
            info.min_domain_value = param->min_domain_value();
            info.max_domain_value = param->max_domain_value();
            infos.push_back(info);
        }
    }
    return infos;
}

inline std::vector<control::PropertyInfo> _read_properties(const Processor* processor)
{
    assert(processor != nullptr);
    std::vector<control::PropertyInfo> infos;
    const auto& params = processor->all_parameters();
    for (const auto& param : params)
    {
        if (param->type() == ParameterType::STRING)
        {
            control::PropertyInfo info;
            info.id = param->id();
            info.label = param->label();
            info.name = param->name();
            infos.push_back(info);
        }
    }
    return infos;
}

ParameterController::ParameterController(BaseEngine* engine) : _event_dispatcher(engine->event_dispatcher()),
                                                               _processors(engine->processor_container())
{}

std::pair<control::ControlStatus, std::vector<control::ParameterInfo>> ParameterController::get_processor_parameters(int processor_id) const
{
    ELKLOG_LOG_DEBUG("get_processor_parameters called with processor {}", processor_id);
    const auto proc = _processors->processor(processor_id);
    if (proc)
    {
        return {control::ControlStatus::OK, _read_parameters(proc.get())};
    }
    return {control::ControlStatus::NOT_FOUND, std::vector<control::ParameterInfo>()};
}

std::pair<control::ControlStatus, std::vector<control::ParameterInfo>> ParameterController::get_track_parameters(int track_id) const
{
    ELKLOG_LOG_DEBUG("get_track_parameters called with processor {}", track_id);
    const auto track = _processors->track(track_id);
    if (track)
    {
        return {control::ControlStatus::OK, _read_parameters(track.get())};
    }
    return {control::ControlStatus::NOT_FOUND, std::vector<control::ParameterInfo>()};
}

std::pair<control::ControlStatus, int> ParameterController::get_parameter_id(int processor_id, const std::string& parameter_name) const
{
    ELKLOG_LOG_DEBUG("get_parameter_id called with processor {} and parameter {}", processor_id, parameter_name);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, 0};
    }
    auto descr = processor->parameter_from_name(parameter_name);
    if (descr != nullptr)
    {
        return {control::ControlStatus::OK, descr->id()};
    }
    return {control::ControlStatus::NOT_FOUND, 0};
}

std::pair<control::ControlStatus, control::ParameterInfo> ParameterController::get_parameter_info(int processor_id, int parameter_id) const
{
    ELKLOG_LOG_DEBUG("get_parameter_info called with processor {} and parameter {}", processor_id, parameter_id);
    control::ParameterInfo info;
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
            info.automatable = descr->automatable();

            return {control::ControlStatus::OK, info};
        }
    }
    return {control::ControlStatus::NOT_FOUND, info};
}

std::pair<control::ControlStatus, float> ParameterController::get_parameter_value(int processor_id, int parameter_id) const
{
    ELKLOG_LOG_DEBUG("get_parameter_value called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {control::ControlStatus::OK, value};
        }
    }
    return {control::ControlStatus::NOT_FOUND, 0};
}

std::pair<control::ControlStatus, float> ParameterController::get_parameter_value_in_domain(int processor_id, int parameter_id) const
{
    ELKLOG_LOG_DEBUG("get_parameter_value_normalised called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_in_domain(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {control::ControlStatus::OK, value};
        }
    }
    return {control::ControlStatus::NOT_FOUND, 0};
}

std::pair<control::ControlStatus, std::string> ParameterController::get_parameter_value_as_string(int processor_id, int parameter_id) const
{
    ELKLOG_LOG_DEBUG("get_parameter_value_as_string called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_formatted(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {control::ControlStatus::OK, value};
        }
    }
    return {control::ControlStatus::NOT_FOUND, ""};
}

std::pair<control::ControlStatus, std::string> ParameterController::get_property_value(int processor_id, int property_id) const
{
    ELKLOG_LOG_DEBUG("get_property_value called with processor {} and property {}", processor_id, property_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->property_value(static_cast<ObjectId>(property_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {control::ControlStatus::OK, value};
        }
    }
    return {control::ControlStatus::NOT_FOUND, ""};
}

control::ControlStatus ParameterController::set_parameter_value(int processor_id, int parameter_id, float value)
{
    float clamped_value = std::clamp<float>(value, 0.0f, 1.0f);
    ELKLOG_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id, clamped_value);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          clamped_value,
                                          IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return control::ControlStatus::OK;
}

control::ControlStatus ParameterController::set_property_value(int processor_id, int property_id, const std::string& value)
{
    ELKLOG_LOG_DEBUG("set_property_value called with processor {}, property {} and value {}", processor_id, property_id, value);
    auto event = new PropertyChangeEvent(static_cast<ObjectId>(processor_id),
                                         static_cast<ObjectId>(property_id),
                                         value,
                                         IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return control::ControlStatus::OK;
}

std::pair<control::ControlStatus, std::vector<control::PropertyInfo>> ParameterController::get_processor_properties(int processor_id) const
{
    ELKLOG_LOG_DEBUG("get_processor_properties called with processor {}", processor_id);
    const auto proc = _processors->processor(processor_id);
    if (proc)
    {
        return {control::ControlStatus::OK, _read_properties(proc.get())};
    }
    return {control::ControlStatus::NOT_FOUND, std::vector<control::PropertyInfo>()};
}

std::pair<control::ControlStatus, std::vector<control::PropertyInfo>> ParameterController::get_track_properties(int track_id) const
{
    ELKLOG_LOG_DEBUG("get_track_properties called with processor {}", track_id);
    const auto track = _processors->track(track_id);
    if (track)
    {
        return {control::ControlStatus::OK, _read_properties(track.get())};
    }
    return {control::ControlStatus::NOT_FOUND, std::vector<control::PropertyInfo>()};
}

std::pair<control::ControlStatus, int> ParameterController::get_property_id(int processor_id, const std::string& property_name) const
{
    ELKLOG_LOG_DEBUG("get_property_id called with processor {} and property {}", processor_id, property_name);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {control::ControlStatus::NOT_FOUND, 0};
    }
    auto descriptor = processor->parameter_from_name(property_name);
    if (descriptor && descriptor->type() == ParameterType::STRING)
    {
        return {control::ControlStatus::OK, descriptor->id()};
    }
    return {control::ControlStatus::NOT_FOUND, 0};
}

std::pair<control::ControlStatus, control::PropertyInfo> ParameterController::get_property_info(int processor_id, int property_id) const
{
    ELKLOG_LOG_DEBUG("get_property_info called with processor {} and parameter {}", processor_id, property_id);
    control::PropertyInfo info;
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto descriptor = processor->parameter_from_id(static_cast<ObjectId>(property_id));
        if (descriptor && descriptor->type() == ParameterType::STRING)
        {
            info.id = descriptor->id();
            info.label = descriptor->label();
            info.name = descriptor->name();
            return {control::ControlStatus::OK, info};
        }
    }
    return {control::ControlStatus::NOT_FOUND, info};
}

} // end namespace sushi::internal::engine::controller_impl
