/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Audio processor with event output example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "peak_meter_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_CHANNELS = 16;
/* Number of updates per second */
constexpr float REFRESH_RATE = 25;
/* fc in Hz, Tweaked by eyeballing mostly */
constexpr float SMOOTHING_CUTOFF = 2.3;
/* Represents -120dB */
constexpr float OUTPUT_MIN = 1.0e-6f;
static const std::string DEFAULT_NAME = "sushi.testing.peakmeter";
static const std::string DEFAULT_LABEL = "Peak Meter";

PeakMeterPlugin::PeakMeterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS;
    _max_output_channels = MAX_CHANNELS;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _left_level = register_float_parameter("left", "Left", "dB", OUTPUT_MIN, OUTPUT_MIN, 1.0f,
                                                new LinTodBPreProcessor(OUTPUT_MIN, 1.0f));
    _right_level = register_float_parameter("right", "Right", "dB", OUTPUT_MIN, OUTPUT_MIN, 1.0f,
                                            new LinTodBPreProcessor(OUTPUT_MIN, 1.0f));
    assert(_left_level && _right_level);
}

void PeakMeterPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    for (int ch = 0; ch < std::min(MAX_METERED_CHANNELS, in_buffer.channel_count()); ++ch)
    {
        float max = 0.0f;
        for (int i{0}; i < AUDIO_CHUNK_SIZE; ++i)
        {
            max = std::max(max, std::abs(in_buffer.channel(ch)[i]));
        }
        _smoothed[ch] = _smoothing_coef * _smoothed[ch] + (1.0f - _smoothing_coef) * max;
    }

    _sample_count += AUDIO_CHUNK_SIZE;
    if (_sample_count > _refresh_interval)
    {
        _sample_count -= _refresh_interval;
        set_parameter_and_notify(_left_level, _smoothed[LEFT_CHANNEL_INDEX]);
        set_parameter_and_notify(_right_level, _smoothed[RIGHT_CHANNEL_INDEX]);
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
    _smoothing_coef = std::exp(-2.0f * M_PI * SMOOTHING_CUTOFF * AUDIO_CHUNK_SIZE/ sample_rate);
}


}// namespace gain_plugin
}// namespace sushi