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

#include "keyboard_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

KeyboardController::KeyboardController(BaseEngine* engine) : _event_dispatcher(engine->event_dispatcher())
{}

ext::ControlStatus KeyboardController::send_note_on(int track_id, int channel, int note, float velocity)
{
    SUSHI_LOG_DEBUG("send_note_on called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, static_cast<ObjectId>(track_id),
                                   channel, note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus KeyboardController::send_note_off(int track_id, int channel, int note, float velocity)
{
    SUSHI_LOG_DEBUG("send_note_off called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, static_cast<ObjectId>(track_id),
                                   channel, note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;}

ext::ControlStatus KeyboardController::send_note_aftertouch(int track_id, int channel, int note, float value)
{
    SUSHI_LOG_DEBUG("send_note_aftertouch called with track {}, note {}, value {}", track_id, note, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   channel, note, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus KeyboardController::send_aftertouch(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_aftertouch called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus KeyboardController::send_pitch_bend(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_pitch_bend called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus KeyboardController::send_modulation(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_modulation called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi
