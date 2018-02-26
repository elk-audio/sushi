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

constexpr int MAX_CHANNELS_SUPPORTED = 2;
static const std::string DEFAULT_NAME = "sushi.testing.equalizer";
static const std::string DEFAULT_LABEL = "Equalizer";

class EqualizerPlugin : public InternalPlugin
{
public:
    EqualizerPlugin(HostControl hostControl);

    ~EqualizerPlugin();

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_input_channels(int channels) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    float _sample_rate;
    dsp::biquad::BiquadFilter _filters[MAX_CHANNELS_SUPPORTED];

    FloatParameterValue* _frequency;
    FloatParameterValue* _gain;
    FloatParameterValue* _q;
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H