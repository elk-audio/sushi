/**
 * @brief Simple 8-step sequencer example.
 * @copyright MIND Music Labs AB, Stockholm
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

class StepSequencerPlugin : public InternalPlugin
{
public:
    explicit StepSequencerPlugin(HostControl host_control);

    ~StepSequencerPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

private:
    std::array<IntParameterValue*, SEQUENCER_STEPS> _pitch_parameters;
    std::array<BoolParameterValue*, SEQUENCER_STEPS> _step_parameters;
    std::array<BoolParameterValue*, SEQUENCER_STEPS> _step_indicator_parameters;
    std::array<int, SEQUENCER_STEPS> _sequence;

    float   _sample_rate;
    int     _current_step{0};
    bool    _current_step_active{true};
    int     _transpose{0};
    int     _current_note{0};

    RtEventFifo _event_queue;
};

float samples_per_qn(float tempo, float samplerate);
int snap_to_scale(int note, const std::array<int, 12>& scale);

}// namespace step_sequencer_plugin
}// namespace sushi

#endif //SUSHI_STEP_SEQUENCER_PLUGIN_H
