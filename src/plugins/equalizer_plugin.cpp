#include <cassert>

#include "equalizer_plugin.h"

namespace sushi {
namespace equalizer_plugin {

EqualizerPlugin::EqualizerPlugin()
{}

EqualizerPlugin::~EqualizerPlugin()
{}

AudioProcessorStatus EqualizerPlugin::init(const AudioProcessorConfig &configuration)
{
    _configuration = configuration;
    _filter.set_smoothing(AUDIO_CHUNK_SIZE);
    _filter.reset();
    return AudioProcessorStatus::OK;
}

void EqualizerPlugin::set_parameter(int parameter_id, float value)
{
    switch (parameter_id)
    {
        case equalizer_parameter_id::FREQUENCY:
        {
            _freq = value;
            break;
        }
        case equalizer_parameter_id::GAIN:
        {
            _gain = value;
            break;
        }
        case equalizer_parameter_id::Q:
        {
            _q = value;
        }
    }
}

void EqualizerPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, this plugin only supports mono in/out. */
    assert(in_buffer->channel_count() == 1);
    assert(out_buffer->channel_count() == 1);

    /* Recalculates the coefficients once per audio chunk, this makes for
     * predictable cpu load for every chunk */

    biquad::BiquadCoefficients coefficients;
    biquad::calc_biquad_peak(coefficients, _configuration.sample_rate, _freq, _q, _gain);
    _filter.set_coefficients(coefficients);
    _filter.process(in_buffer->channel(0), out_buffer->channel(0), AUDIO_CHUNK_SIZE);
}


}// namespace equalizer_plugin
}// namespace sushi