#include "equalizer_plugin.h"

namespace equalizer_plugin {

EqualizerPlugin::EqualizerPlugin()
{}

EqualizerPlugin::~EqualizerPlugin()
{}

AudioProcessorStatus EqualizerPlugin::init(const AudioProcessorConfig& configuration)
{
    _configuration = configuration;
    _filter.set_smoothing(AUDIO_CHUNK_SIZE);
    // _filter.reset();
    return AudioProcessorStatus::OK;
}

void EqualizerPlugin::set_parameter(unsigned int parameter_id, float value)
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

void EqualizerPlugin::process(const float *in_buffer, float *out_buffer)
{
    biquad::BiquadCoefficients coefficients;
    biquad::calc_biquad_peak(coefficients, _configuration.sample_rate, _freq, _q, _gain);
    _filter.set_coefficients(coefficients);
    _filter.process(in_buffer, out_buffer, AUDIO_CHUNK_SIZE);
}


}// namespace equalizer_plugin