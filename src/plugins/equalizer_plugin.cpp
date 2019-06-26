#include <cassert>

#include "equalizer_plugin.h"

namespace sushi {
namespace equalizer_plugin {

EqualizerPlugin::EqualizerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    _current_input_channels = 1;
    _current_output_channels = 1;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _frequency = register_float_parameter("frequency", "Frequency", 1000.0f, 20.0f, 20000.0f,
                                          new FloatParameterPreProcessor(20.0f, 20000.0f));
    _gain = register_float_parameter("gain", "Gain", 0, -24.0f, 24.0f,
                                     new dBToLinPreProcessor(-24.0f, 24.0f));
    _q = register_float_parameter("q", "Q", 1, 0.0f, 10.0f,
                                  new FloatParameterPreProcessor(0.0f, 10.0f));
    assert(_frequency);
    assert(_gain);
    assert(_q);
}

EqualizerPlugin::~EqualizerPlugin()
{}

ProcessorReturnCode EqualizerPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;

    for (auto& f : _filters)
    {
        f.set_smoothing(AUDIO_CHUNK_SIZE);
        f.reset();
    }

    return ProcessorReturnCode::OK;
}

void EqualizerPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    return;
}

void EqualizerPlugin::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    {
        _current_output_channels = channels;
        _max_output_channels = channels;
    }
}

void EqualizerPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    float frequency = _frequency->value();
    float gain = _gain->value();
    float q = _q->value();

    if (!_bypassed)
    {
        /* Recalculate the coefficients once per audio chunk, this makes for
         * predictable cpu load for every chunk */
        dsp::biquad::Coefficients coefficients;
        dsp::biquad::calc_biquad_peak(coefficients, _sample_rate, frequency, q, gain);
        for (int i = 0; i < _current_input_channels; ++i)
        {
            _filters[i].set_coefficients(coefficients);
            _filters[i].process(in_buffer.channel(i), out_buffer.channel(i), AUDIO_CHUNK_SIZE);
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
    }
}

}// namespace equalizer_plugin
}// namespace sushi