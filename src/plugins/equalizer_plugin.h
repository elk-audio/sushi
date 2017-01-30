/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef EQUALIZER_PLUGIN_H
#define EQUALIZER_PLUGIN_H

#include "plugin_interface.h"
#include "biquad_filter.h"

namespace sushi {
namespace equalizer_plugin {

class EqualizerPlugin : public StompBox
{
public:
    EqualizerPlugin();

    ~EqualizerPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    std::string unique_id() const override
    {
        return std::string("sushi.testing.equalizer");
    }

    void process_event(BaseEvent* /*event*/) {}

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

private:
    biquad::BiquadFilter _filter;
    StompBoxConfig _configuration;

    FloatStompBoxParameter* _frequency;
    FloatStompBoxParameter* _gain;
    FloatStompBoxParameter* _q;
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H