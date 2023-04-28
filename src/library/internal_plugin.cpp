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
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "spdlog/fmt/bundled/format.h"

#include "twine/twine.h"

#include "library/internal_plugin.h"

namespace sushi {

InternalPlugin::InternalPlugin(HostControl host_control) : Processor(host_control)
{
    _max_input_channels = DEFAULT_CHANNELS;
    _max_output_channels = DEFAULT_CHANNELS;
    _current_input_channels = DEFAULT_CHANNELS;
    _current_output_channels = DEFAULT_CHANNELS;
}

FloatParameterValue* InternalPlugin::register_float_parameter(const std::string& id,
                                                              const std::string& label,
                                                              const std::string& unit,
                                                              float default_value,
                                                              float min_value,
                                                              float max_value,
                                                              Direction automatable,
                                                              FloatParameterPreProcessor* pre_proc)
{
    if (pre_proc == nullptr)
    {
        pre_proc = new FloatParameterPreProcessor(min_value, max_value);
    }

    auto param = new FloatParameterDescriptor(id, label, unit, min_value, max_value, automatable, pre_proc);

    if (this->register_parameter(param) == false)
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
                                                          Direction automatable,
                                                          IntParameterPreProcessor* pre_proc)
{
    if (pre_proc == nullptr)
    {
         pre_proc = new IntParameterPreProcessor(min_value, max_value);
    }

    auto param = new IntParameterDescriptor(id, label, unit, min_value, max_value, automatable, pre_proc);

    if (this->register_parameter(param) == false)
    {
        return nullptr;
    }

    auto value = ParameterStorage::make_int_parameter_storage(param, default_value, pre_proc);
    /* The parameter id must match the value storage index */
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value);

    return _parameter_values.back().int_parameter_value();
}

BoolParameterValue* InternalPlugin::register_bool_parameter(const std::string& id,
                                                            const std::string& label,
                                                            const std::string& unit,
                                                            bool default_value,
                                                            Direction automatable)
{
    auto param = new BoolParameterDescriptor(id, label, unit, true, false, automatable, nullptr);

    if (!this->register_parameter(param))
    {
        return nullptr;
    }

    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, default_value);
    /* The parameter id must match the value storage index */
    assert(param->id() == _parameter_values.size());
    _parameter_values.push_back(value_storage);

    return _parameter_values.back().bool_parameter_value();
}


bool InternalPlugin::register_property(const std::string& name,
                                       const std::string& label,
                                       const std::string& default_value)
{
    auto param = new StringPropertyDescriptor(name, label, "");

    if (this->register_parameter(param) == false)
    {
        return false;
    }

    // Push a dummy container here for ids to match
    ParameterStorage value_storage = ParameterStorage::make_bool_parameter_storage(param, false);
    _parameter_values.push_back(value_storage);
    // The property value is stored here
    _property_values[param->id()] = default_value;
    return true;
}

void InternalPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::PITCH_BEND:
        case RtEventType::AFTERTOUCH:
        case RtEventType::MODULATION:
        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            /* The default behaviour is to pass keyboard events through unchanged */
            output_event(event);
            break;
        }

        case RtEventType::FLOAT_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::BOOL_PARAMETER_CHANGE:
        {
            /* These are "managed events" where this function provides a default
             * implementation for handling these and setting parameter values */
            _handle_parameter_event(event.parameter_change_event());
            break;
        }

        case RtEventType::SET_STATE:
        {
            auto state = event.processor_state_event()->state();
            _set_rt_state(state);
            async_delete(state);
            break;
        }

        case RtEventType::STRING_PROPERTY_CHANGE:
        {
            /* In order to handle STRING_PROPERTY_CHANGE events in the rt_thread, override
             * process_event() and handle it. Then call this base function which will automatically
             * schedule a delete event that will be executed in the non-rt domain */
            auto typed_event = event.property_change_event();
            async_delete(typed_event->deletable_value());
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
        auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(),
                                                      storage->normalized_value());
        output_event(e);
    }
}

void InternalPlugin::set_parameter_and_notify(IntParameterValue* storage, int new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->normalized_value());
    output_event(e);
}

void InternalPlugin::set_parameter_and_notify(BoolParameterValue* storage, bool new_value)
{
    storage->set(new_value);
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, storage->descriptor()->id(), storage->normalized_value());
    output_event(e);
}

std::pair<ProcessorReturnCode, float> InternalPlugin::parameter_value(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
    }

    const auto& value_storage = _parameter_values[parameter_id];

    if (value_storage.type() == ParameterType::FLOAT)
    {
        float norm_value = value_storage.float_parameter_value()->normalized_value();
        return {ProcessorReturnCode::OK, norm_value};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        float norm_value = value_storage.int_parameter_value()->normalized_value();
        return {ProcessorReturnCode::OK, norm_value};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->domain_value() ? 1.0f : 0.0f};
    }

    return {ProcessorReturnCode::PARAMETER_ERROR, 0.0f};
}

std::pair<ProcessorReturnCode, float> InternalPlugin::parameter_value_in_domain(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
    }

    const auto& value_storage = _parameter_values[parameter_id];

    if (value_storage.type() == ParameterType::FLOAT)
    {
        return {ProcessorReturnCode::OK, value_storage.float_parameter_value()->domain_value()};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        return {ProcessorReturnCode::OK, value_storage.int_parameter_value()->domain_value()};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->domain_value() ? 1.0f : 0.0f};
    }

    return {ProcessorReturnCode::PARAMETER_ERROR, 0};
}

std::pair<ProcessorReturnCode, std::string> InternalPlugin::parameter_value_formatted(ObjectId parameter_id) const
{
    if (parameter_id >= _parameter_values.size())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
    }

    const auto& value_storage = _parameter_values[parameter_id];

    if (value_storage.type() == ParameterType::FLOAT)
    {
        return {ProcessorReturnCode::OK, fmt::format("{0:0.2f}", value_storage.float_parameter_value()->domain_value())};
    }
    else if (value_storage.type() == ParameterType::INT)
    {
        return {ProcessorReturnCode::OK, std::to_string(value_storage.int_parameter_value()->domain_value())};
    }
    else if (value_storage.type() == ParameterType::BOOL)
    {
        return {ProcessorReturnCode::OK, value_storage.bool_parameter_value()->processed_value() ? "True" : "False"};
    }

    return {ProcessorReturnCode::PARAMETER_ERROR, ""};
}

std::pair<ProcessorReturnCode, std::string> InternalPlugin::property_value(ObjectId property_id) const
{
    std::scoped_lock<std::mutex> lock(_property_lock);
    auto node = _property_values.find(property_id);
    if (node == _property_values.end())
    {
        return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
    }
    return {ProcessorReturnCode::OK, node->second};
}

ProcessorReturnCode InternalPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    std::scoped_lock<std::mutex> lock(_property_lock);
    auto node = _property_values.find(property_id);
    if (node == _property_values.end())
    {
        return ProcessorReturnCode::PARAMETER_NOT_FOUND;
    }
    node->second = value;
    _host_control.post_event(new PropertyChangeNotificationEvent(this->id(), property_id, value, IMMEDIATE_PROCESS));
    return ProcessorReturnCode::OK;
}

ProcessorReturnCode InternalPlugin::set_state(ProcessorState* state, bool realtime_running)
{
    for (const auto& property : state->properties())
    {
        this->set_property_value(property.first, property.second);
    }

    if (realtime_running)
    {
        auto rt_state = std::make_unique<RtState>(*state);
        auto event = new RtStateEvent(this->id(), std::move(rt_state), IMMEDIATE_PROCESS);
        _host_control.post_event(event);
    }
    else
    {
        if (state->bypassed().has_value())
        {
            this->set_bypassed(state->bypassed().value());
        }
        for (const auto& parameter : state->parameters())
        {
            auto event = RtEvent::make_parameter_change_event(this->id(), 0, parameter.first, parameter.second);
            this->process_event(event);
        }
        _host_control.post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED,
                                                                 this->id(), 0, IMMEDIATE_PROCESS));
    }
    return ProcessorReturnCode::OK;
}

ProcessorState InternalPlugin::save_state() const
{
    ProcessorState state;
    state.set_bypass(this->bypassed());
    for (const auto& property : this->_property_values)
    {
        state.add_property_change(property.first, property.second);
    }
    for (const auto& parameter : _parameter_values)
    {
        switch (parameter.type())
        {
            case ParameterType::BOOL:
                state.add_parameter_change(parameter.id(), parameter.bool_parameter_value()->normalized_value());
                break;

            case ParameterType::INT:
                state.add_parameter_change(parameter.id(), parameter.int_parameter_value()->normalized_value());
                break;

            case ParameterType::FLOAT:
                state.add_parameter_change(parameter.id(), parameter.float_parameter_value()->normalized_value());
                break;

            default:
                break;
        }
    }
    return state;
}

PluginInfo InternalPlugin::info() const
{
    PluginInfo info;
    info.type = PluginType::INTERNAL;
    info.path = "";
    info.uid = this->uid();
    return info;
}

void InternalPlugin::send_data_to_realtime(BlobData data, int id)
{
    assert(twine::is_current_thread_realtime() == false);
    auto event = new DataPropertyEvent(this->id(), id, data, IMMEDIATE_PROCESS);
    _host_control.post_event(event);
}

void InternalPlugin::send_property_to_realtime(ObjectId property_id, const std::string& value)
{
    assert(twine::is_current_thread_realtime() == false);
    auto event = new StringPropertyEvent(this->id(), property_id, value, IMMEDIATE_PROCESS);
    _host_control.post_event(event);
}

void InternalPlugin::_set_rt_state(const RtState* state)
{
    if (state->bypassed().has_value())
    {
        this->set_bypassed(*state->bypassed());
    }
    for (const auto& parameter : state->parameters())
    {
        auto event = RtEvent::make_parameter_change_event(this->id(), 0, parameter.first, parameter.second);
        this->process_event(event);
    }
    notify_state_change_rt();
}

void InternalPlugin::_handle_parameter_event(const ParameterChangeRtEvent* event)
{
    if (event->param_id() < _parameter_values.size())
    {
        auto storage = &_parameter_values[event->param_id()];

        switch (storage->type())
        {
            case ParameterType::FLOAT:
            {
                auto parameter_value = storage->float_parameter_value();
                if (parameter_value->descriptor()->automatable())
                {
                    parameter_value->set(event->value());
                }
                break;
            }
            case ParameterType::INT:
            {
                auto parameter_value = storage->int_parameter_value();
                if (parameter_value->descriptor()->automatable())
                {
                    parameter_value->set(event->value());
                }
                break;
            }
            case ParameterType::BOOL:
            {
                auto parameter_value = storage->bool_parameter_value();
                if (parameter_value->descriptor()->automatable())
                {
                    parameter_value->set(event->value());
                }
                break;
            }
            default:
                break;
        }
    }
}

} // end namespace sushi
