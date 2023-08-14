/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Flanger from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "flanger_plugin.h"

namespace sushi::internal::flanger_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.flanger";
constexpr auto DEFAULT_LABEL = "Flanger";

constexpr float FLANGER_AMOUNT_SCALE = 0.001f;


FlangerPlugin::FlangerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    // The low-level module bw_chorus.h exposes other parameters
    // (delay & three coefficients for the direct / modulation / feedback branches)

    // but the high-level flanger example pre-configures them accordingly to Dattoro's reccomendations
    _rate = register_float_parameter("rate", "Rate", "Hz",
                                      1.0f, 0.01f, 2.0f,
                                      Direction::AUTOMATABLE,
                                      new CubicWarpPreProcessor(0.01f, 2.0f));
    _amount = register_float_parameter("amount", "Amount", "",
                                       0.0f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));

    assert(_rate);
    assert(_amount);
}

ProcessorReturnCode FlangerPlugin::init(float sample_rate)
{
    // Default values taken from Brickworks example fx_flanger
    bw_chorus_init(&_chorus_coeffs, 0.002f);
    bw_chorus_set_delay(&_chorus_coeffs, 0.001f);
    bw_chorus_set_coeff_x(&_chorus_coeffs, 0.7071f);
    bw_chorus_set_coeff_mod(&_chorus_coeffs, 0.7071f);
    bw_chorus_set_coeff_fb(&_chorus_coeffs, 0.7071f);
    configure(sample_rate);

    return ProcessorReturnCode::OK;
}

void FlangerPlugin::configure(float sample_rate)
{
    bw_chorus_set_sample_rate(&_chorus_coeffs, sample_rate);
    return;
}

void FlangerPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_chorus_reset_coeffs(&_chorus_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        _delay_mem_areas[i].resize(bw_chorus_mem_req(&_chorus_coeffs));
        bw_chorus_mem_set(&_chorus_states[i], _delay_mem_areas[i].data());
    }
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_chorus_reset_state(&_chorus_coeffs, &_chorus_states[i]);
    }
}

void FlangerPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void FlangerPlugin::process_event(const RtEvent& event)
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

void FlangerPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_chorus_set_rate(&_chorus_coeffs, _rate->processed_value());
    bw_chorus_set_amount(&_chorus_coeffs, _amount->processed_value() * FLANGER_AMOUNT_SCALE);

    if (_bypass_manager.should_process())
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

std::string_view FlangerPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::flanger_plugin

