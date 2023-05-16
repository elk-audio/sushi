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
 * @brief Dynamics compressor from Brickworks library
 * @copyright 2017-2023, Elk Audio AB, Stockholm
 */

#include <cassert>

#include "compressor_plugin.h"

namespace sushi {
namespace compressor_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.compressor";
constexpr auto DEFAULT_LABEL = "Compressor";

constexpr float MINUS_3DB = 0.7071067811865476f;

CompressorPlugin::CompressorPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _threshold = register_float_parameter("threshold", "Threshold", "dB",
                                          0.0f, -60.0f, 12.0f,
                                          Direction::AUTOMATABLE,
                                          new FloatParameterPreProcessor(-60.0f, 12.0f));
    _ratio = register_float_parameter("ratio", "Ratio", "",
                                      1.0f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _attack = register_float_parameter("attack", "Attack", "s",
                                       0.0f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));
    _release = register_float_parameter("release", "Release", "s",
                                      0.0f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _gain = register_float_parameter("gain", "Gain", "dB",
                                     0.0f, -60.0f, 60.0f,
                                     Direction::AUTOMATABLE,
                                     new FloatParameterPreProcessor(-60.0f, 60.0f));

    assert(_threshold);
    assert(_ratio);
    assert(_attack);
    assert(_release);
    assert(_gain);
}

ProcessorReturnCode CompressorPlugin::init(float sample_rate)
{
    bw_comp_init(&_compressor_coeffs);
    bw_comp_set_sample_rate(&_compressor_coeffs, sample_rate);
    return ProcessorReturnCode::OK;
}

void CompressorPlugin::configure(float sample_rate)
{
    bw_comp_set_sample_rate(&_compressor_coeffs, sample_rate);
    return;
}

void CompressorPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_comp_reset_coeffs(&_compressor_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_comp_reset_state(&_compressor_coeffs, &_compressor_state[i]);
    }
}

void CompressorPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void CompressorPlugin::process_event(const RtEvent& event)
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

void CompressorPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_comp_set_thresh_dBFS(&_compressor_coeffs, _threshold->processed_value());
    bw_comp_set_ratio(&_compressor_coeffs, _ratio->processed_value());
    bw_comp_set_attack_tau(&_compressor_coeffs, _attack->processed_value());
    bw_comp_set_release_tau(&_compressor_coeffs, _release->processed_value());
    bw_comp_set_gain_dB(&_compressor_coeffs, _gain->processed_value());

    if (_bypass_manager.should_process())
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_comp_update_coeffs_ctrl(&_compressor_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_comp_update_coeffs_audio(&_compressor_coeffs);
            float control_sig = 0.0f;
            float input_samples[_current_input_channels];
            for (int i = 0; i < _current_input_channels; ++i)
            {
                input_samples[i] = *in_channel_ptrs[i]++;
                control_sig += input_samples[i] * MINUS_3DB;
            }

            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_comp_process1(&_compressor_coeffs, &_compressor_state[i],
                                                          input_samples[i], control_sig);
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

std::string_view CompressorPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace compressor_plugin
}// namespace sushi

