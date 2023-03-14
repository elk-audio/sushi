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
 * @brief RT midi frontend, provides a frontend for getting midi messages into the engine
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */

#ifndef SUSHI_RT_MIDI_FRONTEND_H
#define SUSHI_RT_MIDI_FRONTEND_H

#include <map>

#include <rtmidi/RtMidi.h>

#include "base_midi_frontend.h"

namespace sushi {
namespace midi_frontend {

struct RtMidiCallbackData
{
    RtMidiIn midi_input;
    int input_number;
    midi_receiver::MidiReceiver* receiver;
};

class RtMidiFrontend : public BaseMidiFrontend
{
public:
    explicit RtMidiFrontend(int inputs,
                            int outputs,
                            std::vector<std::tuple<int, int, bool>> input_mappings,
                            std::vector<std::tuple<int, int, bool>> output_mappings,
                            midi_receiver::MidiReceiver* receiver);

    ~RtMidiFrontend();

    bool init() override;

    void run() override;

    void stop() override;

    void send_midi(int input, MidiDataByte data, Time timestamp) override;

private:

    int _inputs;
    int _outputs;
    std::vector<std::tuple<int, int, bool>> _input_mappings;
    std::vector<std::tuple<int, int, bool>> _output_mappings;
    std::vector<RtMidiCallbackData> _input_midi_ports;
    std::vector<RtMidiOut> _output_midi_ports;
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_RT_MIDI_FRONTEND_H
