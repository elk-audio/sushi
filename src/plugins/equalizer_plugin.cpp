#include <cassert>

#include "equalizer_plugin.h"

namespace sushi {
namespace equalizer_plugin {

EqualizerPlugin::EqualizerPlugin()
{}

EqualizerPlugin::~EqualizerPlugin()
{}

StompBoxStatus EqualizerPlugin::init(const StompBoxConfig &configuration)
{
    _configuration = configuration;
    _frequency = _configuration.controller->register_float_parameter("Frequency", "frequency", 1000, 20000, 20);
    _gain = _configuration.controller->register_float_parameter("Gain", "gain", 0, 24, -24, new FloatdBToLinPreProcessor(24, -24));
    _q = _configuration.controller->register_float_parameter("Q", "q", 1, 10, 0);

    _filter.set_smoothing(AUDIO_CHUNK_SIZE);
    _filter.reset();
    return StompBoxStatus::OK;
}

void EqualizerPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, this plugin only supports mono in/out. */
    assert(in_buffer->channel_count() == 1);
    assert(out_buffer->channel_count() == 1);

    /* Read the current parameter values */
    float frequency = _frequency->value();
    float gain = _gain->value();
    float q = _q->value();

    /* Recalculates the coefficients once per audio chunk, this makes for
     * predictable cpu load for every chunk */

    biquad::BiquadCoefficients coefficients;
    biquad::calc_biquad_peak(coefficients, _configuration.sample_rate, frequency, q, gain);
    _filter.set_coefficients(coefficients);
    _filter.process(in_buffer->channel(0), out_buffer->channel(0), AUDIO_CHUNK_SIZE);
}


}// namespace equalizer_plugin
}// namespace sushi