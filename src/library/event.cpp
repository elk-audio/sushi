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
 * @brief Main event class used for communication across modules outside the rt part
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "library/event.h"
#include "engine/base_engine.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("event");

/* GCC does not seem to get when a switch case handles all cases */
#pragma GCC diagnostic ignored "-Wreturn-type"

namespace sushi {

Event* Event::from_rt_event(RtEvent& rt_event, Time timestamp)
{
    switch (rt_event.type())
    {
        case RtEventType::NOTE_ON:
        {
            auto typed_ev = rt_event.keyboard_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->note(),
                                     typed_ev->velocity(),
                                     timestamp);
        }
        case RtEventType::NOTE_OFF:
        {
            auto typed_ev = rt_event.keyboard_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->note(),
                                     typed_ev->velocity(),
                                     timestamp);
        }
        case RtEventType::NOTE_AFTERTOUCH:
        {
            auto typed_ev = rt_event.keyboard_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->note(),
                                     typed_ev->velocity(),
                                     timestamp);
        }
        case RtEventType::MODULATION:
        {
            auto typed_ev = rt_event.keyboard_common_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::MODULATION,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->value(),
                                     timestamp);
        }
        case RtEventType::PITCH_BEND:
        {
            auto typed_ev = rt_event.keyboard_common_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->value(),
                                     timestamp);
        }
        case RtEventType::AFTERTOUCH:
        {
            auto typed_ev = rt_event.keyboard_common_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH,
                                     typed_ev->processor_id(),
                                     typed_ev->channel(),
                                     typed_ev->value(),
                                     timestamp);
        }
        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            auto typed_ev = rt_event.wrapped_midi_event();
            return new KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI,
                                     typed_ev->processor_id(),
                                     typed_ev->midi_data(),
                                     timestamp);
        }
        // TODO Fill for all events
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        {
            auto typed_ev = rt_event.parameter_change_event();
            return new ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT,
                                                        typed_ev->processor_id(),
                                                        typed_ev->param_id(),
                                                        typed_ev->value(),
                                                        timestamp);
        }
        case RtEventType::INT_PARAMETER_CHANGE:
        {
            auto typed_ev = rt_event.parameter_change_event();
            return new ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::INT_PARAMETER_CHANGE_NOT,
                                                        typed_ev->processor_id(),
                                                        typed_ev->param_id(),
                                                        typed_ev->value(),
                                                        timestamp);
        }
        case RtEventType::BOOL_PARAMETER_CHANGE:
        {
            auto typed_ev = rt_event.parameter_change_event();
            return new ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::BOOL_PARAMETER_CHANGE_NOT,
                                                        typed_ev->processor_id(),
                                                        typed_ev->param_id(),
                                                        typed_ev->value(),
                                                        timestamp);
        }
        case RtEventType::ASYNC_WORK:
        {
            auto typed_ev = rt_event.async_work_event();
            return new AsynchronousProcessorWorkEvent(typed_ev->callback(),
                                                      typed_ev->callback_data(),
                                                      typed_ev->processor_id(),
                                                      typed_ev->event_id(),
                                                      timestamp);
        }
        case RtEventType::BLOB_DELETE:
        {
            auto typed_ev = rt_event.data_payload_event();
            return new AsynchronousBlobDeleteEvent(typed_ev->value(), timestamp);
        }
        case RtEventType::CLIP_NOTIFICATION:
        {
            auto typed_ev = rt_event.clip_notification_event();
            auto channel_type = typed_ev->channel_type() == ClipNotificationRtEvent::ClipChannelType::INPUT?
                                                            ClippingNotificationEvent::ClipChannelType::INPUT :
                                                            ClippingNotificationEvent::ClipChannelType::OUTPUT;
            return new ClippingNotificationEvent(typed_ev->channel(), channel_type, timestamp);
        }
        default:
            return nullptr;

    }
}

RtEvent KeyboardEvent::to_rt_event(int sample_offset)
{
    switch (_subtype)
    {
        case KeyboardEvent::Subtype::NOTE_ON:
            return RtEvent::make_note_on_event(_processor_id, sample_offset, _channel, _note, _velocity);

        case KeyboardEvent::Subtype::NOTE_OFF:
            return RtEvent::make_note_off_event(_processor_id, sample_offset, _channel, _note, _velocity);

        case KeyboardEvent::Subtype::NOTE_AFTERTOUCH:
            return RtEvent::make_note_aftertouch_event(_processor_id, sample_offset, _channel, _note, _velocity);

        case KeyboardEvent::Subtype::AFTERTOUCH:
            return RtEvent::make_aftertouch_event(_processor_id, sample_offset, _channel, _velocity);

        case KeyboardEvent::Subtype::PITCH_BEND:
            return RtEvent::make_pitch_bend_event(_processor_id, sample_offset, _channel, _velocity);

        case KeyboardEvent::Subtype::MODULATION:
            return RtEvent::make_kb_modulation_event(_processor_id, sample_offset, _channel, _velocity);

        case KeyboardEvent::Subtype::WRAPPED_MIDI:
            return RtEvent::make_wrapped_midi_event(_processor_id, sample_offset, _midi_data);
    }
}

RtEvent ParameterChangeEvent::to_rt_event(int sample_offset)
{
    switch (_subtype)
    {
        case ParameterChangeEvent::Subtype::INT_PARAMETER_CHANGE:
            return RtEvent::make_parameter_change_event(_processor_id, sample_offset, _parameter_id, this->int_value());

        case ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE:
            return RtEvent::make_parameter_change_event(_processor_id, sample_offset, _parameter_id, this->float_value());

        case ParameterChangeEvent::Subtype::BOOL_PARAMETER_CHANGE:
            return RtEvent::make_parameter_change_event(_processor_id, sample_offset, _parameter_id, this->bool_value());

       default:
            /* Only to stop the compiler from complaining */
            return RtEvent();
    }
}

RtEvent SetProcessorBypassEvent::to_rt_event(int /*sample_offset*/)
{
    return RtEvent::make_bypass_processor_event(this->processor_id(), this->bypass_enabled());
}

RtEvent StringPropertyChangeEvent::to_rt_event(int sample_offset)
{
    /* String in RtEvent must be passed as a pointer allocated outside of the event */
    auto string_value = new std::string(_string_value);
    return RtEvent::make_string_parameter_change_event(_processor_id, sample_offset, _parameter_id, string_value);
}

RtEvent DataPropertyChangeEvent::to_rt_event(int sample_offset)
{
    return RtEvent::make_data_parameter_change_event(_processor_id, sample_offset, _parameter_id, _blob_value);
}

int AddTrackEvent::execute(engine::BaseEngine*engine)
{
    auto status = engine->create_track(_name, _channels);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN:
        default:
            return AddTrackEvent::Status::INVALID_NAME;
    }
}

int RemoveTrackEvent::execute(engine::BaseEngine*engine)
{
    auto status = engine->delete_track(_name);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN:
        default:
            return RemoveTrackEvent::Status::INVALID_TRACK;
    }
}

int AddProcessorToTrackEvent::execute(engine::BaseEngine* engine)
{
    engine::PluginType plugin_type;
    switch (_processor_type)
    {
        case AddProcessorToTrackEvent::ProcessorType::INTERNAL:
            plugin_type = engine::PluginType::INTERNAL;
            break;

        case AddProcessorToTrackEvent::ProcessorType::VST2X:
            plugin_type = engine::PluginType::VST2X;
            break;

        case AddProcessorToTrackEvent::ProcessorType::VST3X:
            plugin_type = engine::PluginType::VST3X;
            break;

            // TODO - add LV2 once it has been merged

        default:
            /* GCC is overzealous and warns even with a class enum that plugin_type
             * may be unitialised. This is apparently a by design and not a bug, see:
             * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53479*/
            __builtin_unreachable();
    }

    auto [status, plugin_id] = engine->load_plugin(_uid, _name, _file, plugin_type);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            break;

        case engine::EngineReturnStatus::INVALID_PLUGIN:
            return AddProcessorToTrackEvent::Status::INVALID_NAME;

        case engine::EngineReturnStatus::INVALID_TRACK:
            return AddProcessorToTrackEvent::Status::INVALID_TRACK;

        case engine::EngineReturnStatus::INVALID_PLUGIN_UID:
            return AddProcessorToTrackEvent::Status::INVALID_UID;

        default:
            return EventStatus::ERROR;
    }

    if (_add_to_back)
    {
        SUSHI_LOG_DEBUG("Adding plugin {} to back of track {}", _name, _track);
        status = engine->add_plugin_to_track_back(plugin_id, _track);
    }
    else
    {
        SUSHI_LOG_DEBUG("Adding plugin {} to track {}, before processor {}", _name, _track, _before_processor);
        status = engine->add_plugin_to_track_before(plugin_id, _track, _before_processor);
    }
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_TRACK:
            return AddProcessorToTrackEvent::Status::INVALID_TRACK;

        default:
            return EventStatus::ERROR;
    }
}

int MoveProcessorEvent::execute(engine::BaseEngine* engine)
{
    auto old_plugin_order = engine->processors_on_track(_source_track);

    auto status = engine->remove_plugin_from_track(_processor, _source_track);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            break;

        case engine::EngineReturnStatus::INVALID_PLUGIN:
            return MoveProcessorEvent::Status::INVALID_NAME;

        case engine::EngineReturnStatus::INVALID_TRACK:
            return MoveProcessorEvent::Status::INVALID_SOURCE_TRACK;

        default:
            return EventStatus::ERROR;
    }

    if (_add_to_back)
    {
        status = engine->add_plugin_to_track_back(_processor, _dest_track);
    }
    else
    {
        status = engine->add_plugin_to_track_before(_processor, _dest_track, _before_processor);
    }
    if (status == engine::EngineReturnStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }

    /* If the insertion operation failed, we must put the processor back in the source track */
    if (old_plugin_order.back()->id() == _processor)
    {
        [[maybe_unused]] auto replace_status = engine->add_plugin_to_track_back(_processor, _source_track);
        SUSHI_LOG_WARNING_IF(replace_status != engine::EngineReturnStatus::OK,
                "Failed to replace processor {} on track {}", _processor, _source_track);
    }
    else
    {
        for (auto i = old_plugin_order.begin(); i != old_plugin_order.end(); ++i)
        {
            if ((*i)->id() == _processor)
            {
                i++;
                ObjectId before = (*i)->id();
                [[maybe_unused]] auto replace_status = engine->add_plugin_to_track_before(_processor, _source_track, before);
                SUSHI_LOG_WARNING_IF(replace_status != engine::EngineReturnStatus::OK,
                           "Failed to replace processor {} on track {} before pos {}", _processor, _source_track, before);
                break;
            }
        }
    }
    return MoveProcessorEvent::Status::INVALID_DEST_TRACK;
}

int RemoveProcessorEvent::execute(engine::BaseEngine* engine)
{
    auto status = engine->remove_plugin_from_track(_name, _track);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
        {
            status = engine->delete_plugin(_name);
            if (status == engine::EngineReturnStatus::OK)
            {
                return EventStatus::HANDLED_OK;
            }
            return RemoveProcessorEvent::Status::INVALID_NAME;
        }

        case engine::EngineReturnStatus::INVALID_PLUGIN:
            return RemoveProcessorEvent::Status::INVALID_NAME;

        case engine::EngineReturnStatus::INVALID_TRACK:
        default:
            return RemoveProcessorEvent::Status::INVALID_TRACK;
    }
}

Event* AsynchronousProcessorWorkEvent::execute()
{
    int status = _work_callback(_data, _rt_event_id);
    return new AsynchronousProcessorWorkCompletionEvent(status, _rt_processor, _rt_event_id, IMMEDIATE_PROCESS);
}

RtEvent AsynchronousProcessorWorkCompletionEvent::to_rt_event(int /*sample_offset*/)
{
    return RtEvent::make_async_work_completion_event(_rt_processor, _rt_event_id, _return_value);
}

Event* AsynchronousBlobDeleteEvent::execute()
{
    delete(_data.data);
    return nullptr;
}

int ProgramChangeEvent::execute(engine::BaseEngine* engine)
{
    auto processor = engine->mutable_processor(_processor_id);
    if (processor != nullptr)
    {
        auto status = processor->set_program(_program_no);
        if (status == ProcessorReturnCode::OK)
        {
            return EventStatus::HANDLED_OK;
        }
    }
    return EventStatus::NOT_HANDLED;
}

#pragma GCC diagnostic pop

} // end namespace sushi
