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
 * @brief Comb delay from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "combdelay_plugin.h"

namespace sushi {
namespace comb_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.comb_delay";
constexpr auto DEFAULT_LABEL = "Comb Delay";


CombPlugin::CombPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _ff_delay = register_float_parameter("ff_delay", "Feed-forward Delay", "sec",
                                         0.05f, 0.0f, 1.0f,
                                         Direction::AUTOMATABLE,
                                         new FloatParameterPreProcessor(0.0f, 1.0f));
    _fb_delay = register_float_parameter("fb_delay", "Feedback Delay", "sec",
                                         0.05f, 0.0f, 1.0f,
                                         Direction::AUTOMATABLE,
                                         new FloatParameterPreProcessor(0.0f, 1.0f));
    _blend = register_float_parameter("blend", "Blend", "",
                                      1.0f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _ff_coeff = register_float_parameter("ff_coeff", "Feed-forward Coefficient", "",
                                         0.0f, -1.0f, 1.0f,
                                         Direction::AUTOMATABLE,
                                         new FloatParameterPreProcessor(-1.0f, 1.0f));
    _fb_coeff = register_float_parameter("fb_coeff", "Feedback Coefficient", "",
                                         0.0f, -0.995f, 0.995f,
                                         Direction::AUTOMATABLE,
                                         new FloatParameterPreProcessor(-0.995f, 0.995f));

    assert(_ff_delay);
    assert(_fb_delay);
    assert(_blend);
    assert(_ff_coeff);
    assert(_fb_coeff);
}

ProcessorReturnCode CombPlugin::init(float sample_rate)
{
    // Default values taken from Brickworks example fx_comb
    bw_comb_init(&_comb_coeffs, 1.0f);
    configure(sample_rate);

    return ProcessorReturnCode::OK;
}

void CombPlugin::configure(float sample_rate)
{
    bw_comb_set_sample_rate(&_comb_coeffs, sample_rate);
    return;
}

void CombPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_comb_reset_coeffs(&_comb_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        _delay_mem_areas[i].resize(bw_comb_mem_req(&_comb_coeffs));
        bw_comb_mem_set(&_comb_states[i], _delay_mem_areas[i].data());
    }
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_comb_reset_state(&_comb_coeffs, &_comb_states[i]);
    }
}

void CombPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void CombPlugin::process_event(const RtEvent& event)
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

void CombPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_comb_set_delay_ff(&_comb_coeffs, _ff_delay->processed_value());
    bw_comb_set_delay_fb(&_comb_coeffs, _fb_delay->processed_value());
    bw_comb_set_coeff_blend(&_comb_coeffs, _blend->processed_value());
    bw_comb_set_coeff_ff(&_comb_coeffs, _ff_coeff->processed_value());
    bw_comb_set_coeff_fb(&_comb_coeffs, _fb_coeff->processed_value());

    if (_bypass_manager.should_process())
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_comb_update_coeffs_ctrl(&_comb_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_comb_update_coeffs_audio(&_comb_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_comb_process1(&_comb_coeffs, &_comb_states[i],
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

std::string_view CombPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace comb_plugin
}// namespace sushi

