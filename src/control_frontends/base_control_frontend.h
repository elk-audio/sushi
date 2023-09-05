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
 * This module provides run-time control of the audio engine for parameter
 * changes and plugin control.
 */
#ifndef SUSHI_BASE_CONTROL_FRONTEND_H
#define SUSHI_BASE_CONTROL_FRONTEND_H

#include <string>

#include "library/event_interface.h"
#include "engine/base_engine.h"

namespace sushi::internal::control_frontend {

enum class ControlFrontendStatus
{
    OK,
    ERROR,
    INTERFACE_UNAVAILABLE
};

class BaseControlFrontend : public EventPoster
{
public:
    explicit BaseControlFrontend(engine::BaseEngine* engine) : _engine(engine)
    {
        _event_dispatcher = _engine->event_dispatcher();
    }

    ~BaseControlFrontend() override = default;

    static void completion_callback(void *arg, Event* event, int return_status)
    {
        static_cast<BaseControlFrontend*>(arg)->_completion_callback(event, return_status);
    }

    virtual ControlFrontendStatus init() = 0;

    virtual void run() = 0;

    virtual void stop() = 0;

    void send_parameter_change_event(ObjectId processor, ObjectId parameter, float value);

    void send_string_parameter_change_event(ObjectId processor, ObjectId parameter, const std::string& value);

    void send_keyboard_event(ObjectId processor, KeyboardEvent::Subtype type, int channel, int note, float velocity);

    void send_note_on_event(ObjectId processor, int channel, int note, float velocity);

    void send_note_off_event(ObjectId processor, int channel, int note, float velocity);

    void send_program_change_event(ObjectId processor, int program);

    void send_set_tempo_event(float tempo);

    void send_set_time_signature_event(TimeSignature signature);

    void send_set_playing_mode_event(PlayingMode mode);

    void send_set_sync_mode_event(SyncMode mode);

protected:
    virtual void _completion_callback(Event* event, int return_status) = 0;

    void send_with_callback(std::unique_ptr<Event>&& event);

    engine::BaseEngine* _engine {nullptr};
    dispatcher::BaseEventDispatcher* _event_dispatcher;

};

} // end namespace sushi::internal::control_frontend

#endif // SUSHI_BASE_CONTROL_FRONTEND_H
