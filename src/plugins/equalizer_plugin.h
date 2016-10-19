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

enum equalizer_parameter_id
{
    FREQUENCY = 1,
    GAIN,
    Q
};

class EqualizerPlugin : public StompBox
{
public:
    EqualizerPlugin();

    ~EqualizerPlugin();

    StompBoxStatus init(const StompBoxConfig &configuration) override;

    void set_parameter(int parameter_id, float value) override;

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

private:
    biquad::BiquadFilter _filter;
    StompBoxConfig _configuration;

    float _freq{1000.0f};
    float _gain{0.0f};
    float _q{1.0f};
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H