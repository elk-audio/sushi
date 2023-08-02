/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Saturation from Brickworks library, with internal 2x resampling
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "saturation_plugin.h"

namespace sushi {
namespace saturation_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.saturation";
constexpr auto DEFAULT_LABEL = "Saturation";


SaturationPlugin::SaturationPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _bias = register_float_parameter("bias", "Bias", "",
                                     0.0f, -2.5f, 2.5f,
                                     Direction::AUTOMATABLE,
                                     new FloatParameterPreProcessor(-2.5f, 2.5f));
    _gain = register_float_parameter("gain", "Gain", "",
                                     1.0f, 0.1f, 10.0f,
                                     Direction::AUTOMATABLE,
                                     new CubicWarpPreProcessor(0.1f, 10.0f));

    assert(_bias);
    assert(_gain);
}

ProcessorReturnCode SaturationPlugin::init(float sample_rate)
{
    bw_satur_init(&_saturation_coeffs);
    bw_src_int_init(&_src_up_coeffs, 2);
    bw_src_int_init(&_src_down_coeffs, -2);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void SaturationPlugin::configure(float sample_rate)
{
    bw_satur_set_sample_rate(&_saturation_coeffs, 2.0f * sample_rate);
    return;
}

void SaturationPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_satur_reset_coeffs(&_saturation_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_satur_reset_state(&_saturation_coeffs, &_saturation_states[i]);
        bw_src_int_reset_state(&_src_up_coeffs, &_src_up_states[i], 0.0f);
        bw_src_int_reset_state(&_src_down_coeffs, &_src_down_states[i], 0.0f);
    }
}

void SaturationPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void SaturationPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
    case RtEventType::SET_BYPASS:
    {
        bool bypassed = static_cast<bool>(event.processor_command_event()->value());
        Processor::set_bypassed(bypassed);
        _bypass_manager.set_bypass(bypassed, _sample_rate);
        break;
    }

    default:
        InternalPlugin::process_event(event);
        break;
    }
}

void SaturationPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_satur_set_bias(&_saturation_coeffs, _bias->processed_value());
    bw_satur_set_gain(&_saturation_coeffs, _gain->processed_value());

    if (_bypass_manager.should_process())
    {
        bw_satur_update_coeffs_ctrl(&_saturation_coeffs);
        int n = 0;
        while (n < AUDIO_CHUNK_SIZE)
        {
            // 2x upsample
            int frames_left = std::min(AUDIO_CHUNK_SIZE - n, AUDIO_CHUNK_SIZE >> 1);
            for (int i = 0; i < _current_input_channels; i++)
            {
                bw_src_int_process(&_src_up_coeffs, &_src_up_states[i],
                                   in_buffer.channel(i) + n, _tmp_buf.channel(i), frames_left);
            }
            // upsampled saturation with coefficient interp.
            int frames_upsample = frames_left << 1;
            for (int n_up = 0; n_up < frames_upsample; n_up++)
            {
                bw_satur_update_coeffs_audio(&_saturation_coeffs);
                for (int i = 0; i < _current_input_channels; i++)
                {
                    float* buf = _tmp_buf.channel(i);
                    // we default to the version without gain compensation
                    buf[n_up] = bw_satur_process1(&_saturation_coeffs, &_saturation_states[i], buf[n_up]);
                }
            }
            // 2x downsample
            for (int i = 0; i < _current_input_channels; i++)
            {
                bw_src_int_process(&_src_down_coeffs, &_src_down_states[i],
                                   _tmp_buf.channel(i), out_buffer.channel(i) + n, frames_upsample);
            }
            n += frames_left;
        }
        if (_bypass_manager.should_ramp())
        {
            _bypass_manager.crossfade_output(in_buffer, out_buffer,
                                             _current_input_channels,
                                             _current_output_channels);
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view SaturationPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace saturation_plugin
}// namespace sushi

