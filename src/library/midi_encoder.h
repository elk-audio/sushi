 /**
 * @brief Utility functions for encoding raw midi data.
 *
 */

#ifndef SUSHI_MIDI_ENCODER_H
#define SUSHI_MIDI_ENCODER_H

#include <algorithm>

#include "types.h"

namespace sushi {
namespace midi {

constexpr uint8_t NOTE_OFF_PREFIX   = 0b10000000;
constexpr uint8_t NOTE_ON_PREFIX    = 0b10010000;
constexpr uint8_t POLY_PRES_PREFIX  = 0b10100000;

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


} // end namespace midi
} // end namespace sushi

#endif //SUSHI_MIDI_ENCODER_H
