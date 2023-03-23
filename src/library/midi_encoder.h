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

/**
 * @brief Encode a midi clock start message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_start_message();

/**
 * @brief Encode a midi clock stop message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_stop_message();
/**
 * @brief Encode a midi clock continue message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_continue_message();

/**
 * @brief Encode a midi clock tick message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_timing_clock();

/**
 * @brief Encode a midi active sensing message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_active_sensing();

/**
 * @brief Encode a midi global reset message
 * @return A MidiDataByte containing the encoded message
 */
MidiDataByte encode_reset_message();


} // end namespace midi
} // end namespace sushi

#endif //SUSHI_MIDI_ENCODER_H
