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

    void pause(bool paused) override;

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

    /// For MIDI:
    /////////////////////////////////////////////////////////////

    void receive_midi(int input, MidiDataByte data, Time timestamp) override;
    void set_midi_callback(ReactiveMidiCallback&& callback) override;

    sushi::Time calculate_timestamp_from_start(float sample_rate) const override;
    void increment_samples_since_start(uint64_t sample_count, Time timestamp) override;

private:
    audio_frontend::ReactiveFrontend* _audio_frontend {nullptr};
    midi_frontend::ReactiveMidiFrontend* _midi_frontend {nullptr};
    engine::Transport* _transport {nullptr};
    uint64_t _samples_since_start {0};

    float _tempo {0};
    sushi::TimeSignature _time_signature {0, 0};
    control::PlayingMode _playing_mode {control::PlayingMode::STOPPED};
};

} // end namespace sushi::internal

#endif // REACTIVE_CONTROLLER_H
