/*
* Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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

#ifndef REAL_TIME_CONTROLLER_H
#define REAL_TIME_CONTROLLER_H

#include "control_interface.h"
#include "library/sample_buffer.h"
#include "library/time.h"
#include "library/types.h"

namespace sushi
{

enum class TransportPositionSource
{
    EXTERNAL,
    CALCULATED
};

using PassiveMidiCallback = std::function<void(int output, MidiDataByte data, Time timestamp)>;

/**
 * @brief The API for the methods which can safely be called from a real-time context to interact with Sushi as a library.
 */
class RtController
{
public:
    RtController() = default;
    virtual ~RtController() = default;

    /// For Transport:
    /////////////////////////////////////////////////////////////

    /**
     * @brief Set the tempo of the Sushi transport.
     *        (can be called from a real-time context).
     * @param tempo
     */
    virtual void set_tempo(float tempo) = 0;

    /**
     * @brief Set the time signature of the Sushi transport.
     *        (can be called from a real-time context).
     * @param time_signature
     */
    virtual void set_time_signature(ext::TimeSignature time_signature) = 0;

    /**
     * @brief Set the PlayingMode of the Sushi transport.
     *        (can be called from a real-time context).
     * @param mode
     */
    virtual void set_playing_mode(ext::PlayingMode mode) = 0;

    /// For Audio:
    /////////////////////////////////////////////////////////////

    /**
     * @brief Method to invoke from the host's audio callback.
     * @param channel_count
     * @param timestamp
     */
    virtual void process_audio(int channel_count,
                               Time timestamp) = 0;

    virtual ChunkSampleBuffer& in_buffer() = 0;
    virtual ChunkSampleBuffer& out_buffer() = 0;

    /// For MIDI:
    /////////////////////////////////////////////////////////////

    /**
     * @brief Call to pass MIDI input to Sushi
     * @param input Currently assumed to always be 0 since the frontend only supports a single input device.
     * @param data MidiDataByte
     * @param timestamp Sushi Time timestamp for message
     */
    virtual void receive_midi(int input, MidiDataByte data, Time timestamp) = 0;

    /**
     * @brief Assign a callback which is invoked when a MIDI message is generated from inside Sushi.
     *        (Not safe to call from a real-time context, and should only really be called once).
     * @param callback
     */
    virtual void set_midi_callback(PassiveMidiCallback&& callback) = 0;

    /**
     * @brief If the host doesn't provide a timestamp, this method can be used to calculate it,
     *        based on the sample count from session start.
     * @return The currently calculated Timestamp.
     */
    virtual sushi::Time calculate_timestamp_from_start(float sample_rate) = 0;

    /**
     * @brief Call this at the end of each ProcessBlock, to update the sample count and timestamp used for
     *        time and sample offset calculations.
     * @param sample_count
     * @param timestamp
     */
    virtual void increment_samples_since_start(uint64_t sample_count, Time timestamp) = 0;
};

} // namespace sushi

#endif //REAL_TIME_CONTROLLER_H
