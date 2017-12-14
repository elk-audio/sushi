#include <cstddef>
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

inline bool realtime_msg(uint8_t byte)
{
    return byte & 0b00001000;
}

MessageType decode_common_messages(uint8_t data, size_t size)
{
    uint8_t last_3_bits = data & 0x07;
    switch (last_3_bits)
    {
        case SYSTEM_EX_CODE:
            return MessageType::SYSTEM_EXCLUSIVE;

        case TIME_CODE:
            if (size == 2) return MessageType::TIME_CODE;
            break;

        case SONG_POS_CODE:
            if (size == 3) return MessageType::SONG_POSITION;
            break;

        case SONG_SEL_CODE:
            if (size == 2) return MessageType::SONG_SELECT;
            break;

        case TUNE_REQ_CODE:
            if (size == 1) return MessageType::TUNE_REQUEST;
            break;

        case END_SYSEX_CODE:
            return MessageType::END_OF_EXCLUSIVE;

        default:
            break;
    }
    return MessageType::UNKNOWN;
}


MessageType decode_realtime_message(uint8_t data, size_t size)
{
    if (size != 1)
    {
        return MessageType::UNKNOWN;
    }
    uint8_t last_3_bits = data & 0x07;
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


MessageType decode_control_change_type(const uint8_t* data)
{
    uint8_t controller_no = data[1] & 0x7F;
    if (controller_no < 120)
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
            uint8_t value = data[2] & 0x7F;
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


MessageType decode_message_type(const uint8_t* data, size_t size)
{
    if (size == 0)
    {
        return MessageType::UNKNOWN;
    }
    uint8_t first_4_bits = data[0] >> 4;
    switch (first_4_bits)
    {
        case NOTE_OFF_PREFIX:
            if (size == 3) return MessageType::NOTE_OFF;
            break;

        case NOTE_ON_PREFIX:
            if (size == 3) return MessageType::NOTE_ON;
            break;

        case POLY_PRES_PREFIX:
            if (size == 3) return MessageType::POLY_KEY_PRESSURE;
            break;

        case CTRL_CH_PREFIX:
            if (size == 3) return decode_control_change_type(data);
            break;

        case PROG_CH_PREFIX:
            if (size == 2) return MessageType::PROGRAM_CHANGE;
            break;

        case CHAN_PRES_PREFIX:
            if (size == 2) return MessageType::CHANNEL_PRESSURE;
            break;

        case PITCH_B_PREFIX:
            if (size == 3) return MessageType::PITCH_BEND;
            break;

        case SYSTEM_PREFIX:
            if (realtime_msg(data[0]))
            {
                return decode_realtime_message(data[0], size);
            }
            else
            {
                return decode_common_messages(data[0], size);
            }

        default:
            return MessageType::UNKNOWN;
    }
    return MessageType::UNKNOWN;
}


NoteOffMessage decode_note_off(const uint8_t* data)
{
    NoteOffMessage message;
    message.channel = decode_channel(data[0]);
    message.note = data[1] & 0x7F;
    message.velocity = data[2] & 0x7F;
    return message;
}

NoteOnMessage decode_note_on(const uint8_t* data)
{
    NoteOnMessage message;
    message.channel = decode_channel(data[0]);
    message.note = data[1] & 0x7F;
    message.velocity = data[2] & 0x7F;
    return message;
}

PolyKeyPressureMessage decode_poly_key_pressure(const uint8_t* data)
{
    PolyKeyPressureMessage message;
    message.channel = decode_channel(data[0]);
    message.note = data[1] & 0x7F;
    message.pressure = data[2] & 0x7F;
    return message;
}

ControlChangeMessage decode_control_change(const uint8_t* data)
{
    ControlChangeMessage message;
    message.channel = decode_channel(data[0]);
    message.controller = data[1] & 0x7F;
    message.value = data[2] & 0x7F;
    return message;
}

ProgramChangeMessage decode_program_change(const uint8_t* data)
{
    ProgramChangeMessage message;
    message.channel = decode_channel(data[0]);
    message.program = data[1] & 0x7F;
    return message;
}

ChannelPressureMessage decode_channel_pressure(const uint8_t* data)
{
    ChannelPressureMessage message;
    message.channel = decode_channel(data[0]);
    message.pressure = data[1] & 0x7F;
    return message;
}

PitchBendMessage decode_pitch_bend(const uint8_t* data)
{
    PitchBendMessage message;
    message.channel = decode_channel(data[0]);
    message.value = (data[1] & 0x7F) + ((data[2] & 0x7F) << 7);
    return message;
}

TimeCodeMessage decode_time_code(const uint8_t* data)
{
    TimeCodeMessage message;
    message.message_type = (data[1] & 0x70) >> 4;
    message.value = data[1] & 0x0F;
    return message;
}

SongPositionMessage decode_song_position(const uint8_t* data)
{
    SongPositionMessage message;
    message.beats = (data[1] & 0x7F) + ((data[2] & 0x7F) << 7);
    return message;
}

SongSelectMessage decode_song_select(const uint8_t* data)
{
    SongSelectMessage message;
    message.index = data[1] & 0x7F;
    return message;
}

} // end namspace midi
} // end namespace sushi