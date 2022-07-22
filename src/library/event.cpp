/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "twine/twine.h"

#include "library/event.h"
#include "engine/base_engine.h"
#include "types.h"

/* GCC does not seem to get when a switch case handles all cases */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

namespace sushi {

RtDeletable::~RtDeletable()
{
    assert(twine::is_current_thread_realtime() == false);
}

Event* Event::from_rt_event(const RtEvent& rt_event, Time timestamp)
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
        case RtEventType::TEMPO:
        {
            auto typed_event = rt_event.tempo_event();
            return new TempoNotificationEvent(typed_event->tempo(), timestamp);
        }
        case RtEventType::TIME_SIGNATURE:
        {
            auto typed_event = rt_event.time_signature_event();
            return new TimeSignatureNotificationEvent(typed_event->time_signature(), timestamp);
        }
        case RtEventType::PLAYING_MODE:
        {
            auto typed_event = rt_event.playing_mode_event();
            return new PlayingModeNotificationEvent(typed_event->mode(), timestamp);
        }
        case RtEventType::SYNC_MODE:
        {
            auto typed_event = rt_event.sync_mode_event();
            return new SyncModeNotificationEvent(typed_event->mode(), timestamp);
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
        case RtEventType::DELETE:
        {
            auto typed_ev = rt_event.delete_data_event();
            return new AsynchronousDeleteEvent(typed_ev->data(), timestamp);
        }
        case RtEventType::NOTIFY:
        {
            auto typed_ev = rt_event.processor_notify_event();
            if (typed_ev->action() == ProcessorNotifyRtEvent::Action::PARAMETER_UPDATE)
            {
                return new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED,
                                                       typed_ev->processor_id(),
                                                       0,
                                                       timestamp);
            }
            break;
        }
        case RtEventType::TIMING_TICK:
        {
            auto typed_ev = rt_event.timing_tick_event();
            return new EngineTimingTickNotificationEvent(typed_ev->tick_count(), timestamp);
        }
        default:
            return nullptr;

    }
}

RtEvent KeyboardEvent::to_rt_event(int sample_offset) const
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

RtEvent ParameterChangeEvent::to_rt_event(int sample_offset) const
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

RtEvent SetProcessorBypassEvent::to_rt_event(int /*sample_offset*/) const
{
    return RtEvent::make_bypass_processor_event(this->processor_id(), this->bypass_enabled());
}

RtEvent RtStateEvent::to_rt_event(int /*sample_offset*/) const
{
    // If this is null, then this object has been converted to an RtEvent before, which would imply a larger bug
    assert(_state);
    return RtEvent::make_set_rt_state_event(_processor_id, _state.release());
}

RtStateEvent::RtStateEvent(ObjectId processor_id, std::unique_ptr<RtState> state, Time timestamp) : Event(timestamp),
                                                                                                    _processor_id(processor_id),
                                                                                                    _state(std::move(state))
{}

RtStateEvent::~RtStateEvent() = default;

RtEvent DataPropertyEvent::to_rt_event(int sample_offset) const
{
    return RtEvent::make_data_property_change_event(_processor_id, sample_offset, _property_id, _blob_value);
}

RtEvent StringPropertyEvent::to_rt_event(int sample_offset) const
{
    /* std::string is a too large and complex type to be copied by value into an RtEvent.
     * Instead, copy the string to a heap allocation that will outlive the event.
     * The string should be taken back to the non-rt domain and deleted there. This is handled
     * automatically by InternalPlugins process_event() function */
    auto heap_string = new RtDeletableWrapper<std::string>(_string_value);
    return RtEvent::make_string_property_change_event(_processor_id, sample_offset, _property_id, heap_string);
}

Event* AsynchronousProcessorWorkEvent::execute()
{
    int status = _work_callback(_data, _rt_event_id);
    return new AsynchronousProcessorWorkCompletionEvent(status, _rt_processor, _rt_event_id, IMMEDIATE_PROCESS);
}

RtEvent AsynchronousProcessorWorkCompletionEvent::to_rt_event(int /*sample_offset*/) const
{
    return RtEvent::make_async_work_completion_event(_rt_processor, _rt_event_id, _return_value);
}

Event* AsynchronousBlobDeleteEvent::execute()
{
    delete(_data.data);
    return nullptr;
}

Event* AsynchronousDeleteEvent::execute()
{
    delete _data;
    return nullptr;
}

int ProgramChangeEvent::execute(engine::BaseEngine* engine) const
{
    auto processor = engine->processor_container()->mutable_processor(_processor_id);
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

int PropertyChangeEvent::execute(engine::BaseEngine* engine) const
{
    auto processor = engine->processor_container()->mutable_processor(_processor_id);
    if (processor != nullptr)
    {
        auto status = processor->set_property_value(_property_id, _string_value);
        if (status == ProcessorReturnCode::OK)
        {
            return EventStatus::HANDLED_OK;
        }
    }
    return EventStatus::NOT_HANDLED;
}

int SetEngineTempoEvent::execute(engine::BaseEngine* engine) const
{
    engine->set_tempo(_tempo);
    return 0;
}

int SetEngineTimeSignatureEvent::execute(engine::BaseEngine* engine) const
{
    engine->set_time_signature(_signature);
    return 0;
}

int SetEnginePlayingModeStateEvent::execute(engine::BaseEngine* engine) const
{
    engine->set_transport_mode(_mode);
    return 0;
}

int SetEngineSyncModeEvent::execute(engine::BaseEngine* engine) const
{
    engine->set_tempo_sync_mode(_mode);
    return 0;
}

#pragma GCC diagnostic pop
} // end namespace sushi
