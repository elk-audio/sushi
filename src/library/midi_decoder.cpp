#include <iostream>

#include "library/midi_decoder.h"


namespace sushi {
namespace midi {

constexpr uint8_t NOTE_OFF_PREFIX  = 0b1000;
constexpr uint8_t NOTE_ON_PREFIX   = 0b1001;
constexpr uint8_t POLY_PRES_PREFIX = 0b1010;
constexpr uint8_t CTRL_CH_PREFIX   = 0b1011;
constexpr uint8_t PROG_CH_PREFIX   = 0b1100;
constexpr uint8_t CHAN_PRES_PREFIX = 0b1101;
constexpr uint8_t PITCH_B_PREFIX   = 0b1110;


inline uint8_t decode_channel(uint8_t byte)
{
    return byte & 0x0F;
}



MessageType decode_message_type(const uint8_t* data, size_t size)
{
    if (size == 0)
    {
        return MessageType::UNKNOWN;
    }
    uint8_t first_4_bytes = data[0] >> 4;
    switch (first_4_bytes)
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
            if (size == 3) return MessageType::CONTROL_CHANGE;
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

        default:
            return MessageType::UNSUPPORTED;
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
    message.controller = data[1] & 0x7F; // Nitpick. Controllers 120-127 are reserved for channel mode msgs.
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


} // end namspace midi
} // end namespace sushi