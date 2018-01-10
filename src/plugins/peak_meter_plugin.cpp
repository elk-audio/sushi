#include <cassert>
#include <iostream>

#include "peak_meter_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_CHANNELS = 16;
/* Number of updates per second */
constexpr float REFRESH_RATE = 25;
/* Represents -120dB */
constexpr float OUTPUT_MIN = 0.000001f;
static const std::string DEFAULT_NAME = "sushi.testing.peakmeter";
static const std::string DEFAULT_LABEL = "Peak Meter";

PeakMeterPlugin::PeakMeterPlugin()
{
    _max_input_channels = MAX_CHANNELS;
    _max_output_channels = MAX_CHANNELS;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _left_level = register_float_parameter("left", "Left", OUTPUT_MIN, OUTPUT_MIN, 1.0f,
                                                new LinTodBPreProcessor(OUTPUT_MIN, 1.0f));
    _right_level = register_float_parameter("right", "Right", OUTPUT_MIN, OUTPUT_MIN, 1.0f,
                                            new LinTodBPreProcessor(OUTPUT_MIN, 1.0f));
    assert(_left_level && _right_level);
}

void PeakMeterPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    out_buffer = in_buffer;

    for (int ch = 0; ch < std::min(2, in_buffer.channel_count()); ++ch)
    {
        float max = 0.0f;
        for (int i{0}; i < AUDIO_CHUNK_SIZE; ++i)
        {
            float sample = in_buffer.channel(ch)[i];
            max = std::max(max, sample * sample);
        }
        _smoothed[ch] = _smoothing_coef * max + (1.0f - _smoothing_coef) * _smoothed[ch];
    }

    _sample_count += AUDIO_CHUNK_SIZE;
    if (_sample_count > _refresh_interval)
    {
        _sample_count -= _refresh_interval;
        _left_level->set(_smoothed[LEFT_CHANNEL_INDEX]);
        _right_level->set(_smoothed[RIGHT_CHANNEL_INDEX]);
    }
}

ProcessorReturnCode PeakMeterPlugin::init(float sample_rate)
{
    _update_refresh_interval(sample_rate);
    return ProcessorReturnCode::OK;
}

void PeakMeterPlugin::configure(float sample_rate)
{
    _update_refresh_interval(sample_rate);
}

void PeakMeterPlugin::_update_refresh_interval(float sample_rate)
{
    _refresh_interval = static_cast<int>(std::round(sample_rate / REFRESH_RATE));
    _smoothing_coef = std::exp(-2 * M_PI * (1.0f / AUDIO_CHUNK_SIZE) * _refresh_interval / AUDIO_CHUNK_SIZE);
}


}// namespace gain_plugin
}// namespace sushi