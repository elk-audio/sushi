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
 * @brief 2nd-order multimode filter from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "multi_filter_plugin.h"

namespace sushi::internal::multi_filter_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.multi_filter";
constexpr auto DEFAULT_LABEL = "MultiFilter";


MultiFilterPlugin::MultiFilterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _frequency = register_float_parameter("frequency", "Frequency", "Hz",
                                          1'000.0f, 20.0f, 20'000.0f,
                                          Direction::AUTOMATABLE,
                                          new CubicWarpPreProcessor(20.0f, 20'000.0f));
    _Q = register_float_parameter("Q", "Q", "",
                                  1.0f, 0.5f, 10.0f,
                                  Direction::AUTOMATABLE,
                                  new FloatParameterPreProcessor(0.5f, 10.0f));
    _input_coeff = register_float_parameter("input_coeff", "Input coefficient", "",
                                            1.0f, -1.0f, 1.0f,
                                            Direction::AUTOMATABLE,
                                            new FloatParameterPreProcessor(-1.0f, 1.0f));
    _lowpass_coeff = register_float_parameter("lowpass_coeff", "Lowpass coefficient", "",
                                              0.0f, -1.0f, 1.0f,
                                              Direction::AUTOMATABLE,
                                              new FloatParameterPreProcessor(-1.0f, 1.0f));
    _bandpass_coeff = register_float_parameter("bandpass_coeff", "Bandpass coefficient", "",
                                               0.0f, -1.0f, 1.0f,
                                               Direction::AUTOMATABLE,
                                               new FloatParameterPreProcessor(-1.0f, 1.0f));
    _highpass_coeff = register_float_parameter("highpass_coeff", "Highpass coefficient", "",
                                               0.0f, -1.0f, 1.0f,
                                               Direction::AUTOMATABLE,
                                               new FloatParameterPreProcessor(-1.0f, 1.0f));

    assert(_frequency);
    assert(_Q);
    assert(_input_coeff);
    assert(_lowpass_coeff);
    assert(_bandpass_coeff);
    assert(_highpass_coeff);
}

ProcessorReturnCode MultiFilterPlugin::init(float sample_rate)
{
    bw_mm2_init(&_mm2_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void MultiFilterPlugin::configure(float sample_rate)
{
    bw_mm2_set_sample_rate(&_mm2_coeffs, sample_rate);
    return;
}

void MultiFilterPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_mm2_reset_coeffs(&_mm2_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_mm2_reset_state(&_mm2_coeffs, &_mm2_states[i], 0.0f);
    }
}

void MultiFilterPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void MultiFilterPlugin::process_event(const RtEvent& event)
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

void MultiFilterPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_mm2_set_cutoff(&_mm2_coeffs, _frequency->processed_value());
    bw_mm2_set_Q(&_mm2_coeffs, _Q->processed_value());
    bw_mm2_set_coeff_x(&_mm2_coeffs, _input_coeff->processed_value());
    bw_mm2_set_coeff_lp(&_mm2_coeffs, _lowpass_coeff->processed_value());
    bw_mm2_set_coeff_bp(&_mm2_coeffs, _bandpass_coeff->processed_value());
    bw_mm2_set_coeff_hp(&_mm2_coeffs, _highpass_coeff->processed_value());

    if (_bypass_manager.should_process())
    {
        std::vector<const float *> in_channel_ptrs(_current_input_channels);
        std::vector<float *> out_channel_ptrs(_current_input_channels);

        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_mm2_update_coeffs_ctrl(&_mm2_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_mm2_update_coeffs_audio(&_mm2_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                *out_channel_ptrs[i]++ = bw_mm2_process1(&_mm2_coeffs, &_mm2_states[i],
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

std::string_view MultiFilterPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::multi_filter_plugin


