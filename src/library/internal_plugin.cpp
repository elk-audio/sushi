/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Internal plugin manager.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "library/internal_plugin.h"

namespace sushi {

constexpr int DEFAULT_CHANNELS = 2;

InternalPlugin::InternalPlugin(HostControl host_control) : Processor(host_control)
{
    _max_input_channels = DEFAULT_CHANNELS;
    _max_output_channels = DEFAULT_CHANNELS;
    _current_input_channels = DEFAULT_CHANNELS;
    _current_output_channels = DEFAULT_CHANNELS;
};

FloatParameterValue* InternalPlugin::register_float_parameter(const std::string& id,
                                                              const std::string& label,
                                                              const std::string& unit,
                                                              float default_value,
                                                              float min_value,
                                                              float max_value,
                                                              FloatParameterPreProcessor* pre_proc)
{
    if (pre_proc == nullptr)
    {
        pre_proc = new FloatParameterPreProcessor(min_value, max_value);
    }
    FloatParameterDescriptor* param = new FloatParameterDescriptor(id, label, unit, min_value, max_value, pre_proc);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_float_parameter_storage(param, default_value, pre_proc);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().float_parameter_value();
}

IntParameterValue* InternalPlugin::register_int_parameter(const std::string& id,
                                                          const std::string& label,
                                                          const std::string& unit,
                                                          int default_value,
                                                          int min_value,
                                                          int max_value,
                                                          IntParameterPreProcessor* pre_proc)
{
    if (pre_proc == nullptr)
    {
         pre_proc = new IntParameterPreProcessor(min_value, max_value);
    }
    IntParameterDescriptor* param = new IntParameterDescriptor(id, label, unit, min_value, max_value, pre_proc);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    auto value = ParameterStorage::make_int_parameter_storage(param, default_value,  pre_proc);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);
    return _parameter_values.back().int_parameter_value();
}

BoolParameterValue* InternalPlugin::register_bool_parameter(const std::string& id,
                                                            const std::string& label,
                                                            const std::string& unit,
                                                            bool default_value)
{
    BoolParameterDescriptor* param = new BoolParameterDescriptor(id, label, unit, true, false, nullptr);
    if (!this->register_parameter(param))
    {
        return nullptr;
    }
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, default_value);
    /* The parameter id must match the value storage index*/
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value_storage);
    return _parameter_values.back().bool_parameter_value();
}


bool InternalPlugin::register_string_property(const std::string &id,
                                              const std::string &label,
                                              const std::string& unit)
{
    StringPropertyDescriptor* param = new StringPropertyDescriptor(id, label, unit);
    if (!this->register_parameter(param))
    {
        return false;
    }
    /* We don't provide a string value class but must push a dummy container here for ids to match */
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, false);
    _parameter_values.push_back(value_storage);
    return true;
}


bool InternalPlugin::register_data_property(const std::string &id,
                                            const std::string &label,
                                            const std::string& unit)
{
    DataPropertyDescriptor* param = new DataPropertyDescriptor(id, label, unit);
    if (!this->register_parameter(param))
    {
        return false;
    }
    /* We don't provide a data value class but must push a dummy container here for ids to match */
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, false);
    _parameter_values.push_back(value_storage);
    return true;
}


void InternalPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::BOOL_PARAMETER_CHANGE:
        {
            /* These are "managed events" where this function provides a default
             * implementation for handling these and setting parameter values */
            auto typed_event = event.parameter_change_event();
            if (typed_event->param_id() >= _parameter_values.size())
            {
                break;
            }
            auto storage = &_parameter_values[typed_event->param_id()];
            switch (storage->type())
            {
                case ParameterType::FLOAT:
                {
                    storage->float_parameter_value()->set(typed_event->value());
                    break;
                }
                case ParameterType::INT:
                {
                    storage->int_parameter_value()->set(typed_event->value());
                    break;
                }
                case ParameterType::BOOL:
                {
                    storage->bool_parameter_value()->set_values(typed_event->value(), typed_event->value());
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
}

void InternalPlugin::set_parameter_and_notify(FloatParameterValue* storage, float new_value)
{
    storage->set(new_value);
    if (maybe_output_cv_value(storage->descriptor()->id(), new_value) == false)
    {
        auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
        output_event(e);
    }
}

void InternalPlugin::set_parameter_and_notify(IntParameterValue*storage, int new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
    output_event(e);
}

void InternalPlugin::set_parameter_and_notify(BoolParameterValue*storage, bool new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->value());
    output_event(e);
}

std::pair<ProcessorReturnCode, float> InternalPlugin::parameter_value(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0};
    }
    const auto& value_storage = _parameter_values[parameter_id];
    if (value_storage.type() == ParameterType::FLOAT)
    {
        return {ProcessorReturnCode::OK, value_storage.float_parameter_value()->raw_value()};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        return {ProcessorReturnCode::OK, value_storage.int_parameter_value()->raw_value()};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->raw_value()? 1.0f : 0.0f};
    }
    return {ProcessorReturnCode::PARAMETER_ERROR, 0};
}

std::pair<ProcessorReturnCode, float> InternalPlugin::parameter_value_normalised(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0};
    }
    const auto& value_storage = _parameter_values[parameter_id];
    if (value_storage.type() == ParameterType::FLOAT)
    {
        auto desc = static_cast<FloatParameterDescriptor*>(value_storage.float_parameter_value()->descriptor());
        float value = value_storage.float_parameter_value()->raw_value();
        float norm_value = (value - desc->min_value()) / (desc->max_value() - desc->min_value());
        return {ProcessorReturnCode::OK, norm_value};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        auto desc = static_cast<IntParameterDescriptor*>(value_storage.int_parameter_value()->descriptor());
        float value = value_storage.int_parameter_value()->raw_value();
        float norm_value = (value - desc->min_value()) / static_cast<float>((desc->max_value() - desc->min_value()));
        return {ProcessorReturnCode::OK, norm_value};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->value()? 1.0f : 0.0f};
    }
    return {ProcessorReturnCode::PARAMETER_ERROR, 0};
}

std::pair<ProcessorReturnCode, std::string> InternalPlugin::parameter_value_formatted(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0};
    }
    const auto& value_storage = _parameter_values[parameter_id];
    if (value_storage.type() == ParameterType::FLOAT)
    {
        return {ProcessorReturnCode::OK, std::to_string(value_storage.float_parameter_value()->raw_value())};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        return {ProcessorReturnCode::OK, std::to_string(value_storage.int_parameter_value()->raw_value())};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->value()? "True" : "False"};
    }
    return {ProcessorReturnCode::PARAMETER_ERROR, ""};
}

} // end namespace sushi