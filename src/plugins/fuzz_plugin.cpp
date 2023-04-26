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
 * @brief Fuzz from Brickworks library, with internal 2x resampling
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "fuzz_plugin.h"

namespace sushi {
namespace fuzz_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.fuzz";
constexpr auto DEFAULT_LABEL = "Fuzz";


FuzzPlugin::FuzzPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _fuzz = register_float_parameter("fuzz", "Fuzz", "",
                                     0.0f, 0.0f, 1.0f,
                                     Direction::AUTOMATABLE,
                                     new FloatParameterPreProcessor(0.0f, 1.0f));
    _volume = register_float_parameter("gain", "Gain", "",
                                       1.0f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));

    assert(_fuzz);
    assert(_volume);
}

ProcessorReturnCode FuzzPlugin::init(float sample_rate)
{
    bw_fuzz_init(&_fuzz_coeffs);
    bw_src_int_init(&_src_up_coeffs, 2);
    bw_src_int_init(&_src_down_coeffs, -2);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void FuzzPlugin::configure(float sample_rate)
{
    bw_fuzz_set_sample_rate(&_fuzz_coeffs, 2.0f * sample_rate);
    return;
}

void FuzzPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_fuzz_reset_coeffs(&_fuzz_coeffs);
    for (int i = 0; i < MAX_CHANNELS_SUPPORTED; i++)
    {
        bw_fuzz_reset_state(&_fuzz_coeffs, &_fuzz_states[i]);
        bw_src_int_reset_state(&_src_up_coeffs, &_src_up_states[i], 0.0f);
        bw_src_int_reset_state(&_src_down_coeffs, &_src_down_states[i], 0.0f);
    }
}

void FuzzPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_fuzz_set_fuzz(&_fuzz_coeffs, _fuzz->processed_value());
    bw_fuzz_set_volume(&_fuzz_coeffs, _volume->processed_value());

    if (!_bypassed)
    {
        bw_fuzz_update_coeffs_ctrl(&_fuzz_coeffs);
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
            // upsampled fuzz with coefficient interp.
            int frames_upsample = frames_left << 1;
            for (int n_up = 0; n_up < frames_upsample; n_up++)
            {
                bw_fuzz_update_coeffs_audio(&_fuzz_coeffs);
                for (int i = 0; i < _current_input_channels; i++)
                {
                    float* buf = _tmp_buf.channel(i);
                    buf[n_up] = bw_fuzz_process1(&_fuzz_coeffs, &_fuzz_states[i], buf[n_up]);
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
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view FuzzPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace fuzz_plugin
}// namespace sushi

