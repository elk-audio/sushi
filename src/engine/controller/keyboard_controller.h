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

#ifndef SUSHI_KEYBOARD_CONTROLLER_H
#define SUSHI_KEYBOARD_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class KeyboardController : public ext::KeyboardController
{
public:
    explicit KeyboardController(BaseEngine* engine);

    ~KeyboardController() override = default;

    ext::ControlStatus send_note_on(int track_id, int channel, int note, float velocity) override;

    ext::ControlStatus send_note_off(int track_id, int channel, int note, float velocity) override;

    ext::ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) override;

    ext::ControlStatus send_aftertouch(int track_id, int channel, float value) override;

    ext::ControlStatus send_pitch_bend(int track_id, int channel, float value) override;

    ext::ControlStatus send_modulation(int track_id, int channel, float value) override;

private:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_KEYBOARD_CONTROLLER_H
