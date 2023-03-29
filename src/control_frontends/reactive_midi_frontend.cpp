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
 * @brief Reactive MIDI frontend
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <chrono>

#include "reactive_midi_frontend.h"

#include "library/midi_decoder.h"

#include "sushi/logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("reactive midi frontend");

namespace sushi::internal::midi_frontend {

ReactiveMidiFrontend::ReactiveMidiFrontend(midi_receiver::MidiReceiver* dispatcher)
        : BaseMidiFrontend(dispatcher)
{}

ReactiveMidiFrontend::~ReactiveMidiFrontend()
{
    stop();
}

bool ReactiveMidiFrontend::init()
{
    return true;
}

void ReactiveMidiFrontend::run()
{

}

void ReactiveMidiFrontend::stop()
{

}

void ReactiveMidiFrontend::receive_midi(int input, MidiDataByte data, Time timestamp)
{
    _receiver->send_midi(input, data, timestamp);

    SUSHI_LOG_DEBUG("Received midi message: [{:x} {:x} {:x} {:x}], port{}, timestamp: {}",
                    data[0], data[1], data[2], data[3], input, timestamp.count());
}

void ReactiveMidiFrontend::send_midi(int output, MidiDataByte data, Time timestamp)
{
    if (_callback)
    {
        _callback(output, data, timestamp);
    }
    else
    {
        SUSHI_LOG_DEBUG("ReactiveMidiFrontend::send_midi was invoked on a frontend instance,"
                        " which has no sending _callback. "
                        "First pass one to the frontend using set_callback(...).");
    }
}

void ReactiveMidiFrontend::set_callback(ReactiveMidiCallback&& callback)
{
    _callback = std::move(callback);
}

} // end namespace sushi::internal::midi_frontend

