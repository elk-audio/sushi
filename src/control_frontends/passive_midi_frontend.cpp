/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Passive midi frontend
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <chrono>
#include <cstdlib>

#include "passive_midi_frontend.h"
#include "library/midi_decoder.h"

#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("passive midi");

namespace sushi {
namespace midi_frontend {

constexpr auto ALSA_POLL_TIMEOUT = std::chrono::milliseconds(200);
constexpr auto CLIENT_NAME = "Sushi";

PassiveMidiFrontend::PassiveMidiFrontend(int inputs, int outputs, midi_receiver::MidiReceiver* dispatcher)
        : BaseMidiFrontend(dispatcher),
          _inputs(inputs),
          _outputs(outputs)
{}

PassiveMidiFrontend::~PassiveMidiFrontend()
{
    stop();
}

bool PassiveMidiFrontend::init()
{
    return true;
}

void PassiveMidiFrontend::run()
{

}

void PassiveMidiFrontend::stop()
{

}

void PassiveMidiFrontend::receive_midi(int input, MidiDataByte data, Time timestamp)
{
    _receiver->send_midi(input, data, timestamp);

    SUSHI_LOG_DEBUG("Received midi message: [{:x} {:x} {:x} {:x}], port{}, timestamp: {}",
                    data[0], data[1], data[2], data[3], input, timestamp.count());
}

void PassiveMidiFrontend::send_midi(int output, MidiDataByte data, Time timestamp)
{
    _callback(output, data, timestamp);
}

bool PassiveMidiFrontend::_init_time()
{
    return true;
}

bool PassiveMidiFrontend::_init_ports()
{

    return true;
}

} // end namespace midi_frontend
} // end namespace sushi

