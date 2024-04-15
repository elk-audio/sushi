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
 * @brief Phaser from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "phaser_plugin.h"

namespace sushi::internal::phaser_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.phaser";
constexpr auto DEFAULT_LABEL = "Phaser";


PhaserPlugin::PhaserPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _rate = register_float_parameter("rate", "Rate", "Hz",
                                      1.0f, 0.5f, 5.0f,
                                      Direction::AUTOMATABLE,
                                      new CubicWarpPreProcessor(0.5f, 5.0f));
    _center = register_float_parameter("center", "Center Frequency", "Hz",
                                      1'000.0f, 100.0f, 10'000.0f,
                                      Direction::AUTOMATABLE,
                                      new CubicWarpPreProcessor(100.0f, 10'000.0f));
    _amount = register_float_parameter("amount", "Amount", "oct",
                                       1.0f, 0.0f, 4.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 4.0f));

    assert(_rate);
    assert(_center);
    assert(_amount);
}

ProcessorReturnCode PhaserPlugin::init(float sample_rate)
{
    bw_phaser_init(&_phaser_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void PhaserPlugin::configure(float sample_rate)
{
    bw_phaser_set_sample_rate(&_phaser_coeffs, sample_rate);
    return;
}

void PhaserPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_phaser_reset_coeffs(&_phaser_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_phaser_reset_state(&_phaser_coeffs, &_phaser_states[i]);
    }
}

void PhaserPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void PhaserPlugin::process_event(const RtEvent& event)
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

void PhaserPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_phaser_set_rate(&_phaser_coeffs, _rate->processed_value());
    bw_phaser_set_center(&_phaser_coeffs, _center->processed_value());
    bw_phaser_set_amount(&_phaser_coeffs, _amount->processed_value());

    if (_bypass_manager.should_process())
    {
        std::vector<const float *> in_channel_ptrs(_current_input_channels);
        std::vector<float *> out_channel_ptrs(_current_input_channels);

        // float* in_channel_ptrs[_current_input_channels];
        // float* out_channel_ptrs[_current_input_channels];

        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_phaser_update_coeffs_ctrl(&_phaser_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_phaser_update_coeffs_audio(&_phaser_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_phaser_process1(&_phaser_coeffs, &_phaser_states[i],
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

std::string_view PhaserPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::phaser_plugin
