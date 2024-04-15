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
 * @brief Noise gate from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "noise_gate_plugin.h"

namespace sushi::internal::noise_gate_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.noise_gate";
constexpr auto DEFAULT_LABEL = "Noise gate";


NoiseGatePlugin::NoiseGatePlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _threshold = register_float_parameter("threshold", "Threshold", "dB",
                                      0.0f, -60.0f, 0.0f,
                                      Direction::AUTOMATABLE,
                                      new dBToLinPreProcessor(-60.0f, 0.0f));
    _ratio = register_float_parameter("ratio", "Inverse ratio", "",
                                      0.0f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _attack = register_float_parameter("attack", "Attack time", "sec",
                                       0.0f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));
    _release = register_float_parameter("release", "Release time", "sec",
                                        0.0f, 0.0f, 1.0f,
                                        Direction::AUTOMATABLE,
                                        new FloatParameterPreProcessor(0.0f, 1.0f));

    assert(_threshold);
    assert(_ratio);
    assert(_attack);
    assert(_release);
}

ProcessorReturnCode NoiseGatePlugin::init(float sample_rate)
{
    bw_noise_gate_init(&_noise_gate_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void NoiseGatePlugin::configure(float sample_rate)
{
    bw_noise_gate_set_sample_rate(&_noise_gate_coeffs, sample_rate);
}

void NoiseGatePlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_noise_gate_reset_coeffs(&_noise_gate_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_noise_gate_reset_state(&_noise_gate_coeffs, &_noise_gate_states[i]);
    }
}

void NoiseGatePlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void NoiseGatePlugin::process_event(const RtEvent& event)
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

void NoiseGatePlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_noise_gate_set_thresh_lin(&_noise_gate_coeffs, _threshold->processed_value());
    float ratio_inv = _ratio->processed_value();
    // the Brickworks example uses the INFINITY constant for the upper limit,
    // here only slightly above the previous values
    float ratio = (ratio_inv < 0.999f) ? 1.0f / (1.0f - ratio_inv) : 1.0f / (1.0f - 0.9999f);
    bw_noise_gate_set_ratio(&_noise_gate_coeffs, ratio);
    bw_noise_gate_set_attack_tau(&_noise_gate_coeffs, _attack->processed_value());
    bw_noise_gate_set_release_tau(&_noise_gate_coeffs, _release->processed_value());

    if (_bypass_manager.should_process())
    {
        std::vector<const float *> in_channel_ptrs(_current_input_channels);
        std::vector<float *> out_channel_ptrs(_current_input_channels);

//        const float* in_channel_ptrs[_current_input_channels];
//        float* out_channel_ptrs[_current_input_channels];

        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_noise_gate_update_coeffs_ctrl(&_noise_gate_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_noise_gate_update_coeffs_audio(&_noise_gate_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                float x = *in_channel_ptrs[i]++;
                *out_channel_ptrs[i]++ = bw_noise_gate_process1(&_noise_gate_coeffs, &_noise_gate_states[i],
                                                                x, x);
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

std::string_view NoiseGatePlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::noise_gate_plugin
