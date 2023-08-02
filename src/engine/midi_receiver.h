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
 * @brief Interface class for receiving midi data
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_MIDI_RECEIVER_H
#define SUSHI_MIDI_RECEIVER_H

#include "sushi/sushi_time.h"

namespace sushi::internal::midi_receiver {

class MidiReceiver
{
public:
    virtual void send_midi(int port, MidiDataByte data, Time timestamp) = 0;
};


} // end namespace sushi::internal::midi_dispatcher

#endif // SUSHI_MIDI_RECEIVER_H

