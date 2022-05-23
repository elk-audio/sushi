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

#ifndef PASSIVE_CONTROLLER_H
#define PASSIVE_CONTROLLER_H

#include "real_time_controller.h"
#include "sushi.h"

namespace
{
using TimePoint = std::chrono::nanoseconds;
}

namespace sushi
{

class Sushi;

namespace audio_frontend
{
class PassiveFrontend;
}

namespace midi_frontend
{
class PassiveMidiFrontend;
}

namespace engine
{
class Transport;
}

namespace event_timer
{
class EventTimer;
}

class PassiveController : public RtController
{
public:
    PassiveController();

    ~PassiveController() override;

    InitStatus init(SushiOptions& options);

    /// RtController methods:

    void set_tempo(float tempo) override;

    void set_time_signature(ext::TimeSignature time_signature) override;

    void set_playing_mode(ext::PlayingMode mode) override;

    void set_beat_count(double beat_count) override;
    void set_position_source(TransportPositionSource ps) override;

    void process_audio(int channel_count, Time timestamp) override;

    void receive_midi(int input, MidiDataByte data, Time timestamp) override;

    void set_midi_callback(PassiveMidiCallback&& callback) override;

    ChunkSampleBuffer& in_buffer() override;
    ChunkSampleBuffer& out_buffer() override;

    /// PassiveController methods:

    void set_sample_rate(double sample_rate);
    double sample_rate() const;

    /**
     * @brief If the host doesn't provide a timestamp, this method can be used to calculate it,
     *        based on the sample count from session start.
     * @return The currently calculated Timestamp.
     */
    sushi::Time calculate_timestamp_from_start();

    /**
     * @brief Call this at the end of each ProcessBlock, to update the sample count and timestamp used for
     *        time and sample offset calculations.
     * @param sample_count
     * @param timestamp
     */
    void increment_samples_since_start(uint64_t sample_count, Time timestamp);

    /**
     * @brief Useful for MIDI messaging, to get the timestamp for each MIDI message passed to Sushi.
     * @param offset The sample offset for the MIDI message
     * @return Session time value corresponding to the given sample offset.
     */
    Time real_time_from_sample_offset(int offset) const;

    /**
     * @brief Useful for MIDI messaging, to convert a timestamp to a sample offset within the next chunk.
     * @param timestamp Timestamp for which the sample offset is desired.
     * @return If the timestamp falls withing the next chunk, the function
     *         returns true and the offset in samples. If the timestamp
     *         lies further in the future, the function returns false and
     *         the returned offset is not valid.
     */
    std::pair<bool, int> sample_offset_from_realtime(Time timestamp) const;

private:
    std::unique_ptr<sushi::Sushi> _sushi {nullptr};

    audio_frontend::PassiveFrontend* _audio_frontend {nullptr};
    midi_frontend::PassiveMidiFrontend* _midi_frontend {nullptr};
    sushi::engine::Transport* _transport {nullptr};

    std::unique_ptr<event_timer::EventTimer> _event_timer;
    uint64_t _samples_since_start {0};
    TimePoint _start_time;

    double _sample_rate {0};

    float _tempo {0};
    sushi::TimeSignature _time_signature {0, 0};
    ext::PlayingMode _playing_mode {ext::PlayingMode::STOPPED};
};

} // namespace sushi


#endif // PASSIVE_CONTROLLER_H
