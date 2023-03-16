/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Passive midi frontend, provides a frontend for getting midi messages into the engine
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PASSIVE_MIDI_FRONTEND_H
#define SUSHI_PASSIVE_MIDI_FRONTEND_H

#include <functional>

#include "sushi/time.h"

#include "base_midi_frontend.h"

namespace sushi {
namespace midi_frontend {

/**
 * @brief Callback signature for method to invoke to notify host of any new MIDI message received.
 */
using PassiveMidiCallback = std::function<void(int output, MidiDataByte data, Time timestamp)>;

/**
 * @brief A frontend for MIDI messaging which is to be used when Sushi is included in a hosting audio app/plugin,
 * as a library.
 *
 * The current implementation assumes this will only involve input over a single MIDI device, meaning support for
 * multiple inputs and/or outputs is omitted.
 *
 */
class PassiveMidiFrontend : public BaseMidiFrontend
{
public:
    PassiveMidiFrontend(midi_receiver::MidiReceiver* dispatcher);

    ~PassiveMidiFrontend() override;

    bool init() override;

    void run() override;

    void stop() override;

    void send_midi(int output,
                   MidiDataByte data,
                   [[maybe_unused]]Time timestamp) override;

    /**
     * The embedding host uses this method to pass any incoming MIDI messages to Sushi.
     * @param input Currently assumed to always be 0 since the frontend only supports a single input device.
     * @param data MidiDataByte
     * @param timestamp Sushi Time timestamp for message
     */
    void receive_midi(int input, MidiDataByte data, Time timestamp);

    /**
     * For passing a callback of type PassiveMidiCallback to the frontend.
     * @param callback
     */
    void set_callback(PassiveMidiCallback&& callback);

private:
    PassiveMidiCallback _callback;
};

} // end namespace midi_frontend
} // end namespace sushi

#endif //SUSHI_PASSIVE_MIDI_FRONTEND_H_H
