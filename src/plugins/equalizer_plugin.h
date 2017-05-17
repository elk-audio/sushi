/**
 * @brief Gain plugin example
 * @copyright MIND Music Labs AB, Stockholm
 */
#ifndef EQUALIZER_PLUGIN_H
#define EQUALIZER_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/biquad_filter.h"

namespace sushi {
namespace equalizer_plugin {

static const std::string DEFAULT_NAME = "sushi.testing.equalizer";
static const std::string DEFAULT_LABEL = "Equalizer";

class EqualizerPlugin : public InternalPlugin
{
public:
    EqualizerPlugin();

    ~EqualizerPlugin();

    virtual ProcessorReturnCode init(const int sample_rate) override;

    // void process_event(Event /*event*/) {}

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    float _sample_rate;
    dsp::biquad::BiquadFilter _filter;

    FloatStompBoxParameter* _frequency;
    FloatStompBoxParameter* _gain;
    FloatStompBoxParameter* _q;
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H