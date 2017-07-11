#include <cassert>

#include "equalizer_plugin.h"

namespace sushi {
namespace equalizer_plugin {

EqualizerPlugin::EqualizerPlugin()
{
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

ProcessorReturnCode EqualizerPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;

    _filter.set_smoothing(AUDIO_CHUNK_SIZE);
    _filter.reset();

    return ProcessorReturnCode::OK;
}

EqualizerPlugin::~EqualizerPlugin()
{}

void EqualizerPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* For now, this plugin only supports mono in/out. */
    assert(in_buffer.channel_count() == 1);
    assert(out_buffer.channel_count() == 1);

    /* Read the current parameter values */
    float frequency = _frequency->value();
    float gain = _gain->value();
    float q = _q->value();

    /* Recalculates the coefficients once per audio chunk, this makes for
     * predictable cpu load for every chunk */

    dsp::biquad::Coefficients coefficients;
    dsp::biquad::calc_biquad_peak(coefficients, _sample_rate, frequency, q, gain);
    _filter.set_coefficients(coefficients);
    _filter.process(in_buffer.channel(0), out_buffer.channel(0), AUDIO_CHUNK_SIZE);
}

}// namespace equalizer_plugin
}// namespace sushi