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
 * @brief Wah from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "wah_plugin.h"

namespace sushi::internal::wah_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.wah";
constexpr auto DEFAULT_LABEL = "Wah";


WahPlugin::WahPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _wah = register_float_parameter("wah", "Wah position", "",
                                    0.5f, 0.0f, 1.0f,
                                    Direction::AUTOMATABLE,
                                    new FloatParameterPreProcessor(0.0f, 1.0f));

    assert(_wah);
}

ProcessorReturnCode WahPlugin::init(float sample_rate)
{
    bw_wah_init(&_wah_coeffs);
    bw_wah_set_sample_rate(&_wah_coeffs, sample_rate);
    return ProcessorReturnCode::OK;
}

void WahPlugin::configure(float sample_rate)
{
    bw_wah_set_sample_rate(&_wah_coeffs, sample_rate);
    return;
}

void WahPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_wah_reset_coeffs(&_wah_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_wah_reset_state(&_wah_coeffs, &_wah_states[i]);
    }
}

void WahPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void WahPlugin::process_event(const RtEvent& event)
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

void WahPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_wah_set_wah(&_wah_coeffs, _wah->processed_value());

    if (_bypass_manager.should_process())
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_wah_update_coeffs_ctrl(&_wah_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_wah_update_coeffs_audio(&_wah_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_wah_process1(&_wah_coeffs, &_wah_states[i],
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

std::string_view WahPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::wah_plugin

