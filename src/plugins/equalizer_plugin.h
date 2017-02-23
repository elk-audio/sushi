/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef EQUALIZER_PLUGIN_H
#define EQUALIZER_PLUGIN_H

#include "library/internal_plugin.h"
#include "biquad_filter.h"

namespace sushi {
namespace equalizer_plugin {

class EqualizerPlugin : public InternalPlugin
{
public:
    EqualizerPlugin();

    ~EqualizerPlugin();

    const std::string unique_id() override
    {
        return std::string("sushi.testing.equalizer");
    }

    // void process_event(BaseEvent* /*event*/) {}

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    biquad::BiquadFilter _filter;

    FloatStompBoxParameter* _frequency;
    FloatStompBoxParameter* _gain;
    FloatStompBoxParameter* _q;
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H