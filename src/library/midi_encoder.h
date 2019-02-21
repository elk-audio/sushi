 /**
 * @brief Utility functions for encoding raw midi data.
 *
 */

#ifndef SUSHI_MIDI_ENCODER_H
#define SUSHI_MIDI_ENCODER_H

#include "types.h"

namespace sushi {
namespace midi {

/**
 * @brief Encode a midi note on message
 * @param channel Midi channel to use (0-15)
 * @param note Midi note number
 * @param velocity Velocity (0-1)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_note_on(int channel, int note, float velocity);

/**
 * @brief Encode a midi note off message
 * @param channel Midi channel to use (0-15)
 * @param note Midi note number
 * @param velocity Release velocity (0-1)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_note_off(int channel, int note, float velocity);

/**
 * @brief Encode a polyphonic key pressure message
 * @param channel Midi channel to use (0-15)
 * @param note Midi note number
 * @param pressure Pressure (0-1)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_poly_key_pressure(int channel, int note, float pressure);

/**
 * @brief Encode a control change message
 * @param channel Midi channel to use (0-15)
 * @param controller Midi controller number (0-119)
 * @param value Value to send (0-1)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_control_change(int channel, int controller, float value);

/**
 * @brief Encode a channel pressure (after touch) message
 * @param channel Midi channel to use (0-15)
 * @param pressure Pressure (0-1)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_channel_pressure(int channel, float value);

/**
 * @brief Encode a pitch bend message
 * @param channel Midi channel to use (0-15)
 * @param value Pitch bend value (-1 to 1  where 0 is the middle position)
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_pitch_bend(int channel, float value);

/**
 * @brief Encode a program change message
 * @param channel Midi channel to use (0-15)
 * @param program MIDI program change from 0 to 127
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_program_change(int channel, int program);

} // end namespace midi
} // end namespace sushi

#endif //SUSHI_MIDI_ENCODER_H
