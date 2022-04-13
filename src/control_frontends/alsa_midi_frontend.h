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
 * @brief Alsa midi frontend, provides a frontend for getting midi messages into the engine
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ALSA_MIDI_FRONTEND_H
#define SUSHI_ALSA_MIDI_FRONTEND_H
#ifdef SUSHI_BUILD_WITH_ALSA_MIDI

#include <thread>
#include <atomic>
#include <vector>
#include <map>

#include <alsa/asoundlib.h>

#include "base_midi_frontend.h"
#include "library/time.h"

namespace sushi {
namespace midi_frontend {

constexpr int ALSA_EVENT_MAX_SIZE = 12;

class AlsaMidiFrontend : public BaseMidiFrontend
{
public:
    AlsaMidiFrontend(int inputs, int outputs, midi_receiver::MidiReceiver* dispatcher);

    ~AlsaMidiFrontend() override;

    bool init() override;

    void run() override;

    void stop() override;

    void send_midi(int input, MidiDataByte data, [[maybe_unused]]Time timestamp) override;

private:
    bool _init_ports();
    bool _init_time();
    Time _to_sushi_time(const snd_seq_real_time_t* alsa_time);
    snd_seq_real_time_t _to_alsa_time(Time timestamp);

    void                        _poll_function();
    std::thread                 _worker;
    std::atomic<bool>           _running{false};
    snd_seq_t*                  _seq_handle{nullptr};
    int                         _inputs;
    int                         _outputs;
    std::vector<int>            _input_midi_ports;
    std::vector<int>            _output_midi_ports;
    std::map<int, int>          _port_to_input_map;
    int                         _queue{-1};

    snd_midi_event_t*           _input_parser{nullptr};
    snd_midi_event_t*           _output_parser{nullptr};
    Time                        _time_offset{0};
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_BUILD_WITH_ALSA_MIDI

#endif //SUSHI_ALSA_MIDI_FRONTEND_H_H
