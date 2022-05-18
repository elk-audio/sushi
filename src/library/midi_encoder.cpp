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
 * @brief Utility functions for encoding raw midi data.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cmath>

#include "library/midi_encoder.h"
#include "library/midi_decoder.h"

namespace sushi {
namespace midi {

// Channel messages
constexpr uint8_t NOTE_OFF_PREFIX   = 0b10000000;
constexpr uint8_t NOTE_ON_PREFIX    = 0b10010000;
constexpr uint8_t POLY_PRES_PREFIX  = 0b10100000;
constexpr uint8_t CTRL_CHANGE_PREFIX= 0b10110000;
constexpr uint8_t CHAN_PRES_PREFIX  = 0b11010000;
constexpr uint8_t PITCH_BEND_PREFIX = 0b11100000;
constexpr uint8_t PGM_CHANGE_PREFIX = 0b11000000;

// System real time messages
constexpr uint8_t TIMING_CLOCK_PREFIX   = 0b11111000;
constexpr uint8_t START_PREFIX          = 0b11111010;
constexpr uint8_t CONTINUE_PREFIX       = 0b11111011;
constexpr uint8_t STOP_PREFIX           = 0b11111100;
constexpr uint8_t ACTIVE_SENSING_PREFIX = 0b11111110;
constexpr uint8_t RESET_PREFIX          = 0b11111111;


MidiDataByte encode_note_on(int channel, int note, float velocity)
{
    MidiDataByte data;
    data[0] = NOTE_ON_PREFIX | static_cast<uint8_t>(channel);
    data[1] = note;
    data[2] = static_cast<uint8_t>(std::round(velocity * MAX_VALUE));
    data[3] = 0;
    return data;
}

MidiDataByte encode_note_off(int channel, int note, float velocity)
{
    MidiDataByte data;
    data[0] = NOTE_OFF_PREFIX | static_cast<uint8_t>(channel);
    data[1] = note;
    data[2] = static_cast<uint8_t>(std::round(velocity * MAX_VALUE));
    data[3] = 0;
    return data;
}

MidiDataByte encode_poly_key_pressure(int channel, int note, float pressure)
{
    MidiDataByte data;
    data[0] = POLY_PRES_PREFIX | static_cast<uint8_t>(channel);
    data[1] = note;
    data[2] = static_cast<uint8_t>(std::round(pressure * MAX_VALUE));
    data[3] = 0;
    return data;
}

MidiDataByte encode_control_change(int channel, int controller, float value)
{
    MidiDataByte data;
    data[0] = CTRL_CHANGE_PREFIX | static_cast<uint8_t>(channel);
    data[1] = controller;
    data[2] = static_cast<uint8_t>(std::round(value * MAX_VALUE));
    data[3] = 0;
    return data;
}

MidiDataByte encode_channel_pressure(int channel, float value)
{
    MidiDataByte data;
    data[0] = CHAN_PRES_PREFIX | static_cast<uint8_t>(channel);
    data[1] = static_cast<uint8_t>(std::round(value * MAX_VALUE));
    data[2] = 0;
    data[3] = 0;
    return data;
}

MidiDataByte encode_pitch_bend(int channel, float value)
{
    MidiDataByte data;
    data[0] = PITCH_BEND_PREFIX | static_cast<uint8_t>(channel);
    int pb_val = static_cast<int>((value + 1.0f) * PITCH_BEND_MIDDLE);
    data[1] = static_cast<uint8_t>(pb_val & 0x00FF);
    data[2] = static_cast<uint8_t>( (pb_val & 0xFF00) >> 7 );
    data[3] = 0;
    return data;
}

MidiDataByte encode_program_change(int channel, int program)
{
    MidiDataByte data;
    data[0] = PGM_CHANGE_PREFIX | static_cast<uint8_t>(channel);
    data[1] = static_cast<uint8_t>(program);
    data[2] = 0;
    data[3] = 0;
    return data;
}

MidiDataByte encode_start_message()
{
    return {START_PREFIX, 0, 0, 0};
}

MidiDataByte encode_stop_message()
{
    return {STOP_PREFIX, 0, 0, 0};
}

MidiDataByte encode_continue_message()
{
    return {CONTINUE_PREFIX, 0, 0, 0};
}

MidiDataByte encode_timing_clock()
{
    return {TIMING_CLOCK_PREFIX, 0, 0, 0};
}

MidiDataByte encode_active_sensing()
{
    return {ACTIVE_SENSING_PREFIX, 0, 0, 0};
}

MidiDataByte encode_reset_message()
{
    return {RESET_PREFIX, 0, 0, 0};
}


} // end namespace midi
} // end namespace sushi
