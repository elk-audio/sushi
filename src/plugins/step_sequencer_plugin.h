/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Simple 8-step sequencer example.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_STEP_SEQUENCER_PLUGIN_H
#define SUSHI_STEP_SEQUENCER_PLUGIN_H

#include <array>

#include "library/internal_plugin.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace step_sequencer_plugin {

constexpr int SEQUENCER_STEPS = 8;
constexpr int START_NOTE = 48;
constexpr int NOTE_EVENT_QUEUE_SIZE = 40;

class StepSequencerPlugin : public InternalPlugin, public UidHelper<StepSequencerPlugin>
{
public:
    explicit StepSequencerPlugin(HostControl host_control);

    ~StepSequencerPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    static std::string_view static_uid();

private:
    std::array<IntParameterValue*, SEQUENCER_STEPS>  _pitch_parameters;
    std::array<BoolParameterValue*, SEQUENCER_STEPS> _step_parameters;
    std::array<BoolParameterValue*, SEQUENCER_STEPS> _step_indicator_parameters;
    std::array<int, SEQUENCER_STEPS> _sequence;

    float   _sample_rate;
    int     _current_step{0};
    bool    _current_step_active{true};
    int     _transpose{0};
    int     _current_note{0};

    RtEventFifo<NOTE_EVENT_QUEUE_SIZE> _event_queue;
};

float samples_per_qn(float tempo, float samplerate);
int snap_to_scale(int note, const std::array<int, 12>& scale);

}// namespace step_sequencer_plugin
}// namespace sushi

#endif //SUSHI_STEP_SEQUENCER_PLUGIN_H
