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
 * @brief Tremolo from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "tremolo_plugin.h"

namespace sushi::internal::tremolo_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.tremolo";
constexpr auto DEFAULT_LABEL = "Tremolo";


TremoloPlugin::TremoloPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _rate = register_float_parameter("rate", "Rate", "Hz",
                                      1.0f, 1.0f, 20.0f,
                                      Direction::AUTOMATABLE,
                                      new CubicWarpPreProcessor(1.0f, 20.0f));
    _amount = register_float_parameter("amount", "Amount", "",
                                       1.0f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));

    assert(_rate);
    assert(_amount);
}

ProcessorReturnCode TremoloPlugin::init(float sample_rate)
{
    bw_trem_init(&_trem_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void TremoloPlugin::configure(float sample_rate)
{
    bw_trem_set_sample_rate(&_trem_coeffs, sample_rate);
    return;
}

void TremoloPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_trem_reset_coeffs(&_trem_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_trem_reset_state(&_trem_coeffs, &_trem_states[i]);
    }
}

void TremoloPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void TremoloPlugin::process_event(const RtEvent& event)
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

void TremoloPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_trem_set_rate(&_trem_coeffs, _rate->processed_value());
    bw_trem_set_amount(&_trem_coeffs, _amount->processed_value());

    if (_bypass_manager.should_process())
    {
        std::vector<const float *> in_channel_ptrs(_current_input_channels);
        std::vector<float *> out_channel_ptrs(_current_input_channels);

        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_trem_update_coeffs_ctrl(&_trem_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_trem_update_coeffs_audio(&_trem_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_trem_process1(&_trem_coeffs, &_trem_states[i],
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

std::string_view TremoloPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::tremolo_plugin
