/**
 * @brief Sampler plugin example to test event and sample handling
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SAMPLER_PLUGIN_H
#define SUSHI_SAMPLER_PLUGIN_H

#include <array>

#include "plugin_interface.h"
#include "plugins/sample_player_voice.h"

namespace sushi {
namespace sample_player_plugin {

constexpr size_t TOTAL_POLYPHONY = 8;
// TODO - Fix string parameter support so we can inject the sample path instead
static const std::string SAMPLE_FILE = "../../../test/data/Kawai-K11-GrPiano-C4_mono.wav";

class SamplePlayerPlugin : public StompBox
{
public:
    SamplePlayerPlugin();

    ~SamplePlayerPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    std::string unique_id() const override
    {
        return std::string("sushi.testing.sampleplayer");
    }

    void process_event(BaseMindEvent* event) override ;

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

private:
    int load_sample_file(const std::string& file_name);
    StompBoxConfig _configuration;

    float*  _sample_buffer{nullptr};
    sample_player_voice::Sample _sample;

    SampleBuffer<AUDIO_CHUNK_SIZE> _buffer{1};
    FloatStompBoxParameter* _volume_parameter;
    FloatStompBoxParameter* _attack_parameter;
    FloatStompBoxParameter* _decay_parameter;
    FloatStompBoxParameter* _sustain_parameter;
    FloatStompBoxParameter* _release_parameter;



    std::array<sample_player_voice::Voice, TOTAL_POLYPHONY> _voices;
};

}// namespace sample_player_plugin
}// namespace sushi

#endif //SUSHI_SAMPLER_PLUGIN_H
