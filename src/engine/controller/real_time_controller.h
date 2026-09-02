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

#ifndef REACTIVE_CONTROLLER_H
#define REACTIVE_CONTROLLER_H

#include "sushi/rt_controller.h"
#include "sushi/sushi.h"
#include "engine/base_event_dispatcher.h"

namespace sushi::internal {

class ConcreteSushi;

namespace audio_frontend
{
class ReactiveFrontend;
}

namespace midi_frontend
{
class ReactiveMidiFrontend;
}

namespace engine
{
class Transport;
}

class RtControllerAccessor;

/**
 * @brief When a host application embeds Sushi, it should use this class to interface with Sushi in a real-time context.
 *        RealTimeController implements the RtController API.
 */
class RealTimeController : public RtController
{
public:
    RealTimeController(audio_frontend::ReactiveFrontend* audio_frontend,
                       midi_frontend::ReactiveMidiFrontend* midi_frontend,
                       engine::Transport* transport);

    ~RealTimeController() override = default;

    /// For Transport:
    /////////////////////////////////////////////////////////////

    void set_tempo(float tempo) override;

    void set_time_signature(control::TimeSignature time_signature) override;

    void set_playing_mode(control::PlayingMode mode) override;

    bool set_current_beats(double beat_count) override;

    bool set_current_bar_beats(double bar_beat_count) override;

    void set_position_source(TransportPositionSource ps) override;

    /// For Audio:
    /////////////////////////////////////////////////////////////

    void process_audio(ChunkSampleBuffer& in_buffer,
                       ChunkSampleBuffer& out_buffer,
                       Time timestamp) override;

    void notify_interrupted_audio(sushi::Time duration) override;

    /// For CV and Gate I/O:
    /////////////////////////////////////////////////////////////

    void set_cv_input(int channel, float value) override;
    [[nodiscard]] float cv_output(int channel) const override;
    void set_gate_input(int gate, bool high) override;
    [[nodiscard]] bool gate_output(int gate) const override;

    /// For MIDI:
    /////////////////////////////////////////////////////////////

    void receive_midi(int input, MidiDataByte data, Time timestamp) override;
    void set_midi_callback(ReactiveMidiCallback&& callback) override;

    [[nodiscard]] sushi::Time calculate_timestamp_from_start(float sample_rate) const override;
    void increment_samples_since_start(int64_t sample_count, Time timestamp) override;

private:
    friend RtControllerAccessor;

    audio_frontend::ReactiveFrontend* _audio_frontend {nullptr};
    midi_frontend::ReactiveMidiFrontend* _midi_frontend {nullptr};
    engine::Transport* _transport {nullptr};
    int64_t _samples_since_start {0};

    // Real-clock anchor for calculate_timestamp_from_start().
    // Set by increment_samples_since_start() when the host supplies a non-zero
    // hardware timestamp (e.g. twine::current_rt_time()).  When set, the utility
    // anchors its output to the real clock instead of a pure sample counter,
    // which fixes Ableton Link sync and prevents float-precision drift after
    // long sessions.
    Time    _clock_anchor {0};
    int64_t _samples_at_anchor {0};

    float _tempo {0};
    sushi::TimeSignature _time_signature {0, 0};
    control::PlayingMode _playing_mode {control::PlayingMode::STOPPED};
};

} // end namespace sushi::internal

#endif // REACTIVE_CONTROLLER_H
