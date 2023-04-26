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
 * @brief Chorus from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cstdlib>
#include <cassert>

#include "chorus_plugin.h"

namespace sushi {
namespace chorus_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.chorus";
constexpr auto DEFAULT_LABEL = "Chorus";

// just a little more margin than the default 16 on most platform
// to take better advantage of AVX etc. when supported by the compiler
constexpr size_t DELAY_LINE_MEMALIGN = 32;


ChorusPlugin::ChorusPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    // The low-level module bw_chorus.h exposes other parameters
    // (delay & three coefficients for the direct / modulation / feedback branches)

    // but the high-level chorus example pre-configures them accordingly to Dattoro's reccomendations
    _rate = register_float_parameter("rate", "Rate", "Hz",
                                      1.0f, 0.01f, 2.0f,
                                      Direction::AUTOMATABLE,
                                      new CubicWarpPreProcessor(0.01f, 2.0f));
    _amount = register_float_parameter("amount", "Amount", "",
                                       0.0f, 0.0f, 0.004f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 0.004f));

    assert(_rate);
    assert(_amount);
}

ChorusPlugin::~ChorusPlugin()
{
    for (int i = 0; i < MAX_CHANNELS_SUPPORTED; i++)
    {
        if (_delay_mem_areas[i])
        {
            free(_delay_mem_areas[i]);
        }
    }
}

ProcessorReturnCode ChorusPlugin::init(float sample_rate)
{
    // Default values taken from Brickworks example fx_chorus
    bw_chorus_init(&_chorus_coeffs, 0.01f);
	bw_chorus_set_delay(&_chorus_coeffs, 0.005f);
	bw_chorus_set_coeff_x(&_chorus_coeffs, 0.7071f);
	bw_chorus_set_coeff_mod(&_chorus_coeffs, 1.f);
	bw_chorus_set_coeff_fb(&_chorus_coeffs, -0.7071f);
    configure(sample_rate);

    // The Brickworks VST3 example does alloc/dealloc of the delay lines
    // at every change of setEnabled, but since we're not changing the delay values
    // we can do everything here instead
    for (int i = 0; i < MAX_CHANNELS_SUPPORTED; i++)
    {
        auto memret = posix_memalign(&_delay_mem_areas[i], DELAY_LINE_MEMALIGN, bw_chorus_mem_req(&_chorus_coeffs));
        if (memret != 0)
        {
            return ProcessorReturnCode::MEMORY_ERROR;
        }
        bw_chorus_mem_set(&_chorus_states[i], _delay_mem_areas[i]);
    }
    return ProcessorReturnCode::OK;
}

void ChorusPlugin::configure(float sample_rate)
{
    bw_chorus_set_sample_rate(&_chorus_coeffs, sample_rate);
    return;
}

void ChorusPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_chorus_reset_coeffs(&_chorus_coeffs);
    for (int i = 0; i < MAX_CHANNELS_SUPPORTED; i++)
    {
        bw_chorus_reset_state(&_chorus_coeffs, &_chorus_states[i]);
    }
}

void ChorusPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_chorus_set_rate(&_chorus_coeffs, _rate->processed_value());
    bw_chorus_set_amount(&_chorus_coeffs, _amount->processed_value());

    if (!_bypassed)
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_chorus_update_coeffs_ctrl(&_chorus_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_chorus_update_coeffs_audio(&_chorus_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_chorus_process1(&_chorus_coeffs, &_chorus_states[i],
                                                            *in_channel_ptrs[i]++);
            }
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view ChorusPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace chorus_plugin
}// namespace sushi

