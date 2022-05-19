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

#ifndef NIKKEI_REAL_TIME_CONTROLLER_H
#define NIKKEI_REAL_TIME_CONTROLLER_H

#include "logging.h"

// TODO AUD-426: Clean up, move to include/Sushi for some?
#include "control_interface.h"

#include "library/constants.h"
#include "library/sample_buffer.h"
#include "library/time.h"
#include "library/types.h"

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

using PassiveMidiCallback = std::function<void(int output, MidiDataByte data, Time timestamp)>;

// TODO AUD-426: Merge with MAX_FRONTEND_CHANNELS in base_audio_frontend, make part of public interface.
constexpr int MAX_FRONTEND_CHANNELS = 8;

// TODO AUD-426: Do we really need an abstract base class for this? If so, where should it be?
class RtController
{
public:
    RtController() = default;
    virtual ~RtController() = default;

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

    /**
     * @brief Set the beat count of the Sushi transport.
     *        (can be called from a real-time context).
     * @param beat_time
     */
    virtual void set_beat_count(double beat_time) = 0;

    /**
     * @brief Method to invoke from the host's audio callback.
     * @param channel_count
     * @param sample_count
     * @param timestamp
     */
    virtual void process_audio(int channel_count,
                               int64_t sample_count,
                               Time timestamp) = 0;

    // TODO AUD-426: Expand interface once below is done.
};

// TODO AUD-426: If it gets many more responsibilities than only RT,
//  We can call it passive controller?
class RealTimeController : public RtController
{
public:
    RealTimeController(Sushi* sushi);

    ~RealTimeController() override;

    void init();

    void set_tempo(float tempo) override;

    void set_time_signature(ext::TimeSignature time_signature) override;

    void set_playing_mode(ext::PlayingMode mode) override;

    void set_beat_count(double beat_count) override;

    void process_audio(int channel_count,
                       int64_t sample_count,
                       Time timestamp) override;

    /**
     * @brief Call to pass MIDI input to Sushi
     * @param input Currently assumed to always be 0 since the frontend only supports a single input device.
     * @param data MidiDataByte
     * @param timestamp Sushi Time timestamp for message
     */
    void receive_midi(int input, MidiDataByte data, Time timestamp);

    /**
     * @brief Assign a callback which is invoked when a MIDI message is generated from inside Sushi.
     *        (Not safe to call from a real-time context, and should only really be called once).
     * @param callback
     */
    void set_midi_callback(PassiveMidiCallback&& callback);

    ChunkSampleBuffer& in_buffer();
    ChunkSampleBuffer& out_buffer();

    // TODO AUD-426: Why did I write this if it's unused?
    sushi::Time timestamp_from_clock();

    sushi::Time timestamp_from_start() const;

    uint64_t samples_since_start() const;
    void increment_samples_since_start(uint64_t amount);

    void set_sample_rate(double sample_rate);
    double sample_rate() const;

    // TODO AUD-426: Should I still be exposing these? Really?
    void set_incoming_time(Time timestamp);
    void set_outgoing_time(Time timestamp);

    Time real_time_from_sample_offset(int offset);
    std::pair<bool, int> sample_offset_from_realtime(Time timestamp);

    // TODO AUD-426: Avoid this duplication of PositionSource
    enum class PositionSource
    {
        EXTERNAL,
        CALCULATED
    };

    void set_position_source(PositionSource ps);

private:
    Sushi* _sushi {nullptr};

    audio_frontend::PassiveFrontend* _audio_frontend {nullptr};
    midi_frontend::PassiveMidiFrontend* _midi_frontend {nullptr};
    sushi::engine::Transport* _transport {nullptr};

    ChunkSampleBuffer _in_buffer {MAX_FRONTEND_CHANNELS};
    ChunkSampleBuffer _out_buffer {MAX_FRONTEND_CHANNELS};

    // TODO AUD-426: Currently, this is an instance owned here - independent of Sushi's.
    //   Since it may only be needed in the host plugin,
    //   I didn't immediately expose Sushi's embedded instance.
    std::unique_ptr<event_timer::EventTimer> _event_timer;

    uint64_t _samples_since_start {0};
    TimePoint _start_time;
    bool _is_start_time_set {false};

    double _sample_rate {0};

    float _tempo {0};
    sushi::TimeSignature _time_signature {0, 0};
    ext::PlayingMode _playing_mode {ext::PlayingMode::STOPPED};
};

} // namespace sushi

#endif //NIKKEI_REAL_TIME_CONTROLLER_H
