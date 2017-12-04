#include <cmath>

#include "library/midi_encoder.h"
#include "library/midi_decoder.h"

namespace sushi {
namespace midi {

constexpr uint8_t NOTE_OFF_PREFIX   = 0b10000000;
constexpr uint8_t NOTE_ON_PREFIX    = 0b10010000;
constexpr uint8_t POLY_PRES_PREFIX  = 0b10100000;
constexpr uint8_t CTRL_CHANGE_PREFIX= 0b10110000;
constexpr uint8_t CHAN_PRES_PREFIX  = 0b11010000;
constexpr uint8_t PITCH_BEND_PREFIX = 0b11100000;


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

MidiDataByte encode_channel_pressure(int channel, float pressure)
{
    MidiDataByte data;
    data[0] = CHAN_PRES_PREFIX | static_cast<uint8_t>(channel);
    data[1] = static_cast<uint8_t>(std::round(pressure * MAX_VALUE));
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


} // end namespace midi
} // end namespace sushi
