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

#ifndef SUSHI_KEYBOARD_CONTROLLER_H
#define SUSHI_KEYBOARD_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi::internal::engine::controller_impl {

class KeyboardController : public control::KeyboardController
{
public:
    explicit KeyboardController(BaseEngine* engine);

    ~KeyboardController() override = default;

    control::ControlStatus send_note_on(int track_id, int channel, int note, float velocity) override;

    control::ControlStatus send_note_off(int track_id, int channel, int note, float velocity) override;

    control::ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) override;

    control::ControlStatus send_aftertouch(int track_id, int channel, float value) override;

    control::ControlStatus send_pitch_bend(int track_id, int channel, float value) override;

    control::ControlStatus send_modulation(int track_id, int channel, float value) override;

private:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_KEYBOARD_CONTROLLER_H
