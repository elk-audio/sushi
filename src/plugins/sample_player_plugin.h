/**
 * @brief Sampler plugin example to test event and sample handling
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SAMPLER_PLUGIN_H
#define SUSHI_SAMPLER_PLUGIN_H

#include <array>

#include "library/internal_plugin.h"
#include "plugins/sample_player_voice.h"

namespace sushi {
namespace sample_player_plugin {

constexpr size_t TOTAL_POLYPHONY = 8;
// TODO - Fix string parameter support so we can inject the sample path instead
// (and have it relative to project structure, right now this fails when built in another location)
static const std::string SAMPLE_FILE = "../../../test/data/Kawai-K11-GrPiano-C4_mono.wav";

static const std::string DEFAULT_NAME = "sushi.testing.sampleplayer";
static const std::string DEFAULT_LABEL = "Sample player";

class SamplePlayerPlugin : public InternalPlugin
{
public:
    SamplePlayerPlugin();

    ~SamplePlayerPlugin();

    virtual ProcessorReturnCode init(const int sample_rate) override;

    void process_event(Event event) override ;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    ProcessorReturnCode load_sample_file(const std::string &file_name);

    float*  _sample_buffer{nullptr};
    dsp::Sample _sample;

    SampleBuffer<AUDIO_CHUNK_SIZE> _buffer{1};
    FloatParameterValue* _volume_parameter;
    FloatParameterValue* _attack_parameter;
    FloatParameterValue* _decay_parameter;
    FloatParameterValue* _sustain_parameter;
    FloatParameterValue* _release_parameter;
    std::string _sample_file_property;

    std::array<sample_player_voice::Voice, TOTAL_POLYPHONY> _voices;
};


}// namespace sample_player_plugin
}// namespace sushi

#endif //SUSHI_SAMPLER_PLUGIN_H
