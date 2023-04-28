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
 * @brief 1 band equaliser plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "equalizer_plugin.h"

namespace sushi {
namespace equalizer_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.equalizer";
constexpr auto DEFAULT_LABEL = "Equalizer";

EqualizerPlugin::EqualizerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _frequency = register_float_parameter("frequency", "Frequency", "Hz",
                                          1000.0f, 20.0f, 20000.0f,
                                          Direction::AUTOMATABLE,
                                          new FloatParameterPreProcessor(20.0f, 20000.0f));

    _gain = register_float_parameter("gain", "Gain", "dB",
                                     0.0f, -24.0f, 24.0f,
                                     Direction::AUTOMATABLE,
                                     new dBToLinPreProcessor(-24.0f, 24.0f));

    _q = register_float_parameter("q", "Q", "",
                                  1.0f, 0.0f, 10.0f,
                                  Direction::AUTOMATABLE,
                                  new FloatParameterPreProcessor(0.0f, 10.0f));
    assert(_frequency);
    assert(_gain);
    assert(_q);
}

ProcessorReturnCode EqualizerPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;
    _reset_filters();
    return ProcessorReturnCode::OK;
}

void EqualizerPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    _reset_filters();
    return;
}

void EqualizerPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    _reset_filters();
}

void EqualizerPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    float frequency = _frequency->processed_value();
    float gain = _gain->processed_value();
    float q = _q->processed_value();

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
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view EqualizerPlugin::static_uid()
{
    return PLUGIN_UID;
}

void EqualizerPlugin::_reset_filters()
{
    for (auto& filter : _filters)
    {
        filter.set_smoothing(AUDIO_CHUNK_SIZE);
        filter.reset();
    }
}

}// namespace equalizer_plugin
}// namespace sushi