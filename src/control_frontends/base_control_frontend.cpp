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
 * @brief Base class for control frontend
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "library/midi_encoder.h"
#include "control_frontends/base_control_frontend.h"

namespace sushi::internal::control_frontend {

void BaseControlFrontend::send_parameter_change_event(ObjectId processor,
                                                      ObjectId parameter,
                                                      float value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    _event_dispatcher->post_event(std::make_unique<ParameterChangeEvent>(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                                                         processor, parameter, value, timestamp));
}

void BaseControlFrontend::send_string_parameter_change_event(ObjectId processor,
                                                             ObjectId parameter,
                                                             const std::string& value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    _event_dispatcher->post_event(std::make_unique<PropertyChangeEvent>(processor, parameter, value, timestamp));
}


void BaseControlFrontend::send_keyboard_event(ObjectId processor,
                                              KeyboardEvent::Subtype type,
                                              int channel,
                                              int note,
                                              float velocity)
{
    Time timestamp = IMMEDIATE_PROCESS;
    _event_dispatcher->post_event(std::make_unique<KeyboardEvent>(type, processor, channel, note, velocity, timestamp));
}

void BaseControlFrontend::send_note_on_event(ObjectId processor, int channel, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_ON, channel, note, velocity);
}

void BaseControlFrontend::send_note_off_event(ObjectId processor, int channel, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_OFF, channel, note, velocity);
}

void BaseControlFrontend::send_program_change_event(ObjectId processor, int program)
{
    Time timestamp = IMMEDIATE_PROCESS;
    _event_dispatcher->post_event(std::make_unique<ProgramChangeEvent>(processor, program, timestamp));
}

void BaseControlFrontend::send_set_tempo_event(float tempo)
{
    _event_dispatcher->post_event(std::make_unique<SetEngineTempoEvent>(tempo, IMMEDIATE_PROCESS));
}

void BaseControlFrontend::send_set_time_signature_event(TimeSignature signature)
{
    _event_dispatcher->post_event(std::make_unique<SetEngineTimeSignatureEvent>(signature, IMMEDIATE_PROCESS));
}

void BaseControlFrontend::send_set_playing_mode_event(PlayingMode mode)
{
    _event_dispatcher->post_event(std::make_unique<SetEnginePlayingModeStateEvent>(mode, IMMEDIATE_PROCESS));
}

void BaseControlFrontend::send_set_sync_mode_event(SyncMode mode)
{
    _event_dispatcher->post_event(std::make_unique<SetEngineSyncModeEvent>(mode, IMMEDIATE_PROCESS));
}

void BaseControlFrontend::send_with_callback(std::unique_ptr<Event>&& event)
{
    event->set_completion_cb(BaseControlFrontend::completion_callback, this);
    _event_dispatcher->post_event(std::move(event));
}

} // end namespace sushi::internal::control_frontend
