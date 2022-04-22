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
 * @brief Passive midi frontend, provides a frontend for getting midi messages into the engine
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PASSIVE_MIDI_FRONTEND_H
#define SUSHI_PASSIVE_MIDI_FRONTEND_H

#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <functional>

#include "base_midi_frontend.h"
#include "library/time.h"

namespace sushi {
namespace midi_frontend {

using PassiveMidiCallback = std::function<void(int output, MidiDataByte data, Time timestamp)>;

class PassiveMidiFrontend : public BaseMidiFrontend
{
public:
    PassiveMidiFrontend(int inputs, int outputs, midi_receiver::MidiReceiver* dispatcher);

    ~PassiveMidiFrontend() override;

    bool init() override;

    void run() override;

    void stop() override;

    void send_midi(int input, MidiDataByte data, [[maybe_unused]]Time timestamp) override;

    void receive_midi(int input, MidiDataByte data, Time timestamp);

    void set_callback(PassiveMidiCallback&& callback)
    {
        _callback = std::move(callback);
    }

private:
    bool _init_ports();
    bool _init_time();

    int _inputs;
    int _outputs;
    std::vector<int> _input_midi_ports;
    std::vector<int> _output_midi_ports;
    std::map<int, int> _port_to_input_map;
    int _queue {-1};

    Time _time_offset {0};

    PassiveMidiCallback _callback;
};

} // end namespace midi_frontend
} // end namespace sushi

#endif //SUSHI_PASSIVE_MIDI_FRONTEND_H_H
