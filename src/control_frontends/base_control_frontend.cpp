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
* @brief Base class for control frontend
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#include <thread>

#include "logging.h"

#include "library/midi_encoder.h"
#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

SUSHI_GET_LOGGER;

constexpr int STOP_RETRIES = 200;
constexpr auto RETRY_INTERVAL = std::chrono::milliseconds(2);

void BaseControlFrontend::send_parameter_change_event(ObjectId processor,
                                                      ObjectId parameter,
                                                      float value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                      processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_string_parameter_change_event(ObjectId processor,
                                                             ObjectId parameter,
                                                             const std::string& value)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new StringPropertyChangeEvent(processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);}


void BaseControlFrontend::send_keyboard_event(ObjectId processor,
                                              KeyboardEvent::Subtype type,
                                              int channel,
                                              int note,
                                              float velocity)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new KeyboardEvent(type, processor, channel, note, velocity, timestamp);
    _event_dispatcher->post_event(e);
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
    auto e = new ProgramChangeEvent(processor, program, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_add_track_event(const std::string &name, int channels)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new AddTrackEvent(name, channels, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_track_event(const std::string &name)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new RemoveTrackEvent(name, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_add_processor_event(const std::string &track, const std::string &uid,
                                                   const std::string &name, const std::string &file,
                                                   AddProcessorToTrackEvent::ProcessorType type)
{
    Time timestamp = IMMEDIATE_PROCESS;
    // TODO - deprecate This entire class
    auto e = new AddProcessorToTrackEvent(uid, name, file, type, 0, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_processor_event(const std::string &track, const std::string &name)
{
    Time timestamp = IMMEDIATE_PROCESS;
    auto e = new RemoveProcessorEvent(name, track, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_set_tempo_event(float tempo)
{
    auto e = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_time_signature_event(TimeSignature signature)
{
    auto e = new SetEngineTimeSignatureEvent(signature, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_playing_mode_event(PlayingMode mode)
{
    auto e = new SetEnginePlayingModeStateEvent(mode, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_set_sync_mode_event(SyncMode mode)
{
    auto e = new SetEngineSyncModeEvent(mode, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_with_callback(Event* event)
{
    event->set_completion_cb(BaseControlFrontend::completion_callback, this);
    _event_dispatcher->post_event(event);
}

} // end namespace control_frontend
} // end namespace sushi
