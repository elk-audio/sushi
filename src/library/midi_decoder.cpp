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
 * @brief Utility functions for decoding raw midi data.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cstdint>

#include "library/midi_decoder.h"

namespace sushi {
namespace midi {

constexpr uint8_t NOTE_OFF_PREFIX   = 0b1000;
constexpr uint8_t NOTE_ON_PREFIX    = 0b1001;
constexpr uint8_t POLY_PRES_PREFIX  = 0b1010;
constexpr uint8_t CTRL_CH_PREFIX    = 0b1011;
constexpr uint8_t PROG_CH_PREFIX    = 0b1100;
constexpr uint8_t CHAN_PRES_PREFIX  = 0b1101;
constexpr uint8_t PITCH_B_PREFIX    = 0b1110;
constexpr uint8_t SYSTEM_PREFIX     = 0b1111;

constexpr uint8_t SYSTEM_EX_CODE    = 0b0000;
constexpr uint8_t TIME_CODE         = 0b0001;
constexpr uint8_t SONG_POS_CODE     = 0b0010;
constexpr uint8_t SONG_SEL_CODE     = 0b0011;
constexpr uint8_t TUNE_REQ_CODE     = 0b0110;
constexpr uint8_t END_SYSEX_CODE    = 0b0111;

constexpr uint8_t TIMING_CLOCK_CODE = 0b0000;
constexpr uint8_t START_CMD_CODE    = 0b0010;
constexpr uint8_t CONTINUE_CMD_CODE = 0b0011;
constexpr uint8_t STOP_CMD_CODE     = 0b0100;
constexpr uint8_t ACTIVE_SENSING    = 0b0110;
constexpr uint8_t RESET_CODE        = 0b0111;

constexpr uint8_t SOUND_OFF_CTRL    = 120;
constexpr uint8_t RESET_CTRL        = 121;
constexpr uint8_t LOCAL_CTRL        = 122;
constexpr uint8_t NOTES_OFF_CTRL    = 123;
constexpr uint8_t OMNI_OFF_CTRL     = 124;
constexpr uint8_t OMNI_ON_CTRL      = 125;
constexpr uint8_t MONO_MODE_CTRL    = 126;
constexpr uint8_t POLY_MODE_CTRL    = 127;

constexpr uint8_t STRIP_MSG_BIT     = 0x7Fu;
constexpr uint8_t STRIP_4_MSG_BITS  = 0x0Fu;
constexpr uint8_t STRIP_4_LSG_BITS  = 0xF7u;
constexpr uint8_t STRIP_5_MSG_BITS  = 0x07u;

inline bool realtime_msg(MidiDataByte data)
{
    return (data[0] & 0b00001000u) > 0;
}

MessageType decode_common_messages(MidiDataByte data)
{
    uint8_t last_3_bits = data[0] & STRIP_5_MSG_BITS;
    switch (last_3_bits)
    {
        case SYSTEM_EX_CODE:
            return MessageType::SYSTEM_EXCLUSIVE;

        case TIME_CODE:
            return MessageType::TIME_CODE;

        case SONG_POS_CODE:
            return MessageType::SONG_POSITION;

        case SONG_SEL_CODE:
            return MessageType::SONG_SELECT;

        case TUNE_REQ_CODE:
            return MessageType::TUNE_REQUEST;

        case END_SYSEX_CODE:
            return MessageType::END_OF_EXCLUSIVE;

        default:
            break;
    }
    return MessageType::UNKNOWN;
}


MessageType decode_realtime_message(MidiDataByte data)
{
    uint8_t last_3_bits = data[0] & STRIP_5_MSG_BITS;
    switch(last_3_bits)
    {
        case TIMING_CLOCK_CODE:
            return MessageType::TIMING_CLOCK;

        case START_CMD_CODE:
            return MessageType::START;

        case CONTINUE_CMD_CODE:
            return MessageType::CONTINUE;

        case STOP_CMD_CODE:
            return MessageType::STOP;

        case ACTIVE_SENSING:
            return MessageType::ACTIVE_SENSING;

        case RESET_CODE:
            return MessageType::RESET;

        default:
            return MessageType::UNKNOWN;
    }
}


MessageType decode_control_change_type(MidiDataByte data)
{
    uint8_t controller_no = data[1] & STRIP_MSG_BIT;
    if (controller_no <= MAX_CONTROLLER_NO)
    {
        return MessageType::CONTROL_CHANGE;
    }
    switch (controller_no)
    {
        case SOUND_OFF_CTRL:
            return MessageType::ALL_SOUND_OFF;

        case RESET_CTRL:
            return MessageType::RESET_ALL_CONTROLLERS;

        case LOCAL_CTRL:
        {
            uint8_t value = data[2] & STRIP_MSG_BIT;
            switch(value)
            {
                case 0:
                    return MessageType::LOCAL_CONTROL_OFF;

                case 127:
                    return MessageType::LOCAL_CONTROL_ON;

                default:
                    return MessageType::UNKNOWN;
            }
        }

        case NOTES_OFF_CTRL:
            return MessageType::ALL_NOTES_OFF;

        case OMNI_OFF_CTRL:
            return MessageType::OMNI_MODE_OFF;

        case OMNI_ON_CTRL:
            return MessageType::OMNI_MODE_ON;

        case MONO_MODE_CTRL:
            return MessageType::MONO_MODE_ON;

        case POLY_MODE_CTRL:
            return MessageType::POLY_MODE_ON;

        default:
            return MessageType::UNKNOWN;
    }
}


MessageType decode_message_type(MidiDataByte data)
{
    uint8_t first_4_bits = data[0] >> 4u;
    switch (first_4_bits)
    {
        case NOTE_OFF_PREFIX:
            return MessageType::NOTE_OFF;

        case NOTE_ON_PREFIX:
            return MessageType::NOTE_ON;

        case POLY_PRES_PREFIX:
            return MessageType::POLY_KEY_PRESSURE;

        case CTRL_CH_PREFIX:
            return decode_control_change_type(data);

        case PROG_CH_PREFIX:
            return MessageType::PROGRAM_CHANGE;

        case CHAN_PRES_PREFIX:
            return MessageType::CHANNEL_PRESSURE;

        case PITCH_B_PREFIX:
            return MessageType::PITCH_BEND;

        case SYSTEM_PREFIX:
            if (realtime_msg(data))
            {
                return decode_realtime_message(data);
            }
            else
            {
                return decode_common_messages(data);
            }

        default:
            return MessageType::UNKNOWN;
    }
}


NoteOffMessage decode_note_off(MidiDataByte data)
{
    NoteOffMessage message;
    message.channel = decode_channel(data);
    message.note = data[1] & STRIP_MSG_BIT;
    message.velocity = data[2] & STRIP_MSG_BIT;
    return message;
}

NoteOnMessage decode_note_on(MidiDataByte data)
{
    NoteOnMessage message;
    message.channel = decode_channel(data);
    message.note = data[1] & STRIP_MSG_BIT;
    message.velocity = data[2] & STRIP_MSG_BIT;
    return message;
}

PolyKeyPressureMessage decode_poly_key_pressure(MidiDataByte data)
{
    PolyKeyPressureMessage message;
    message.channel = decode_channel(data);
    message.note = data[1] & STRIP_MSG_BIT;
    message.pressure = data[2] & STRIP_MSG_BIT;
    return message;
}

ControlChangeMessage decode_control_change(MidiDataByte data)
{
    ControlChangeMessage message;
    message.channel = decode_channel(data);
    message.controller = data[1] & STRIP_MSG_BIT;
    message.value = data[2] & STRIP_MSG_BIT;
    return message;
}

ProgramChangeMessage decode_program_change(MidiDataByte data)
{
    ProgramChangeMessage message;
    message.channel = decode_channel(data);
    message.program = data[1] & STRIP_MSG_BIT;
    return message;
}

ChannelPressureMessage decode_channel_pressure(MidiDataByte data)
{
    ChannelPressureMessage message;
    message.channel = decode_channel(data);
    message.pressure = data[1] & STRIP_MSG_BIT;
    return message;
}

PitchBendMessage decode_pitch_bend(MidiDataByte data)
{
    PitchBendMessage message;
    message.channel = decode_channel(data);
    message.value = (data[1] & STRIP_MSG_BIT) + ((data[2] & STRIP_MSG_BIT) << 7);
    return message;
}

TimeCodeMessage decode_time_code(MidiDataByte data)
{
    TimeCodeMessage message;
    message.message_type = (data[1] & (STRIP_MSG_BIT & STRIP_4_LSG_BITS)) >> 4;
    message.value = data[1] & STRIP_4_MSG_BITS;
    return message;
}

SongPositionMessage decode_song_position(MidiDataByte data)
{
    SongPositionMessage message;
    message.beats = (data[1] & STRIP_MSG_BIT) + ((data[2] & STRIP_MSG_BIT) << 7);
    return message;
}

SongSelectMessage decode_song_select(MidiDataByte data)
{
    SongSelectMessage message;
    message.index = data[1] & STRIP_MSG_BIT;
    return message;
}

} // end namspace midi
} // end namespace sushi