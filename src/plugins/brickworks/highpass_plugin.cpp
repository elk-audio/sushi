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
 * @brief HighPass from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "highpass_plugin.h"

namespace sushi::internal::highpass_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.highpass";
constexpr auto DEFAULT_LABEL = "HighPass";


HighPassPlugin::HighPassPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _frequency = register_float_parameter("frequency", "Frequency", "Hz",
                                          50.0f, 20.0f, 20'000.0f,
                                          Direction::AUTOMATABLE,
                                          new CubicWarpPreProcessor(20.0f, 20'000.0f));

    assert(_frequency);
}

ProcessorReturnCode HighPassPlugin::init(float sample_rate)
{
    bw_hp1_init(&_hp1_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void HighPassPlugin::configure(float sample_rate)
{
    bw_hp1_set_sample_rate(&_hp1_coeffs, sample_rate);
    return;
}

void HighPassPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_hp1_reset_coeffs(&_hp1_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_hp1_reset_state(&_hp1_coeffs, &_hp1_states[i], 0.0f);
    }
}

void HighPassPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void HighPassPlugin::process_event(const RtEvent& event)
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

void HighPassPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_hp1_set_cutoff(&_hp1_coeffs, _frequency->processed_value());

    if (!_bypassed)
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_hp1_update_coeffs_ctrl(&_hp1_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_hp1_update_coeffs_audio(&_hp1_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_hp1_process1(&_hp1_coeffs, &_hp1_states[i],
                                                         *in_channel_ptrs[i]++);
            }
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view HighPassPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::highpass_plugin

