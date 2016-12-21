/**
 * @brief Sampler plugin example to test event and sample handling
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SAMPLER_PLUGIN_H
#define SUSHI_SAMPLER_PLUGIN_H


#include "plugin_interface.h"

namespace sushi {
namespace sample_player_plugin {

class SamplePlayerPlugin : public StompBox
{
public:
    SamplePlayerPlugin();

    ~SamplePlayerPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    std::string unique_id() const override
    {
        return std::string("sushi.testing.sampler");
    }

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

private:
    StompBoxConfig _configuration;

    FloatStompBoxParameter* _volume_parameter;
};

}// namespace sample_player_plugin
}// namespace sushi

#endif //SUSHI_SAMPLER_PLUGIN_H
