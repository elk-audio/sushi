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
 * @brief Base class for midi frontend
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */

#ifndef SUSHI_BASE_MIDI_FRONTEND_H
#define SUSHI_BASE_MIDI_FRONTEND_H

#include "library/types.h"
#include "engine/midi_receiver.h"

namespace sushi {
namespace midi_frontend {

class BaseMidiFrontend
{
public:
    explicit BaseMidiFrontend(midi_receiver::MidiReceiver* receiver) : _receiver(receiver) {}

    virtual ~BaseMidiFrontend() = default;

    virtual bool init() = 0;

    virtual void run() = 0;

    virtual void stop() = 0;

    virtual void send_midi(int input, MidiDataByte data, Time timestamp) = 0;

protected:
    midi_receiver::MidiReceiver* _receiver;
};

/**
 * @brief A no-op implementation of the MidiFrontend. Simply discards all midi inputs sent to it.
 *        Useful for dummy and offline audio frontends.
 */
class NullMidiFrontend : public BaseMidiFrontend
{
public:
    explicit NullMidiFrontend(midi_receiver::MidiReceiver* receiver) : BaseMidiFrontend(receiver) {}
    explicit NullMidiFrontend(int /* input */, int /* outputs */, midi_receiver::MidiReceiver* receiver) : BaseMidiFrontend(receiver) {}

    bool init() override {return true;};

    void run() override {};

    void stop() override {};

    void send_midi(int /*input*/, MidiDataByte /*data*/, Time /*timestamp*/) override {};
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_BASE_MIDI_FRONTEND_H
