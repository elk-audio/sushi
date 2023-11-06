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
 * @brief 3-band equalizer from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "eq3band_plugin.h"

namespace sushi::internal::eq3band_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.eq3band";
constexpr auto DEFAULT_LABEL = "3-band Equalizer";


Eq3bandPlugin::Eq3bandPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _lowshelf_freq = register_float_parameter("lowshelf_freq", "Low-shelf Frequency", "Hz",
                                              125.0f, 25.0f, 1'000.0f,
                                              Direction::AUTOMATABLE,
                                              new CubicWarpPreProcessor(25.0f, 1'000.0f));
    _lowshelf_gain = register_float_parameter("lowshelf_gain", "Low-shelf Gain", "dB",
                                              0.0f, -24.0f, 24.0f,
                                              Direction::AUTOMATABLE,
                                              new dBToLinPreProcessor(-24.0f, 24.0f));
    _lowshelf_q = register_float_parameter("lowshelf_q", "Low-shelf Q", "",
                                           1.0f, 0.5f, 5.0f,
                                           Direction::AUTOMATABLE,
                                           new FloatParameterPreProcessor(0.5f, 5.0f));

    _peak_freq = register_float_parameter("peak_freq", "Peak frequency", "Hz",
                                          1'000.0f, 25.0f, 20'000.0f,
                                          Direction::AUTOMATABLE,
                                          new CubicWarpPreProcessor(25.0f, 20'000.0f));
    _peak_gain = register_float_parameter("peak_gain", "Peak Gain", "dB",
                                          0.0f, -24.0f, 24.0f,
                                          Direction::AUTOMATABLE,
                                          new dBToLinPreProcessor(-24.0f, 24.0f));
    _peak_q = register_float_parameter("peak_q", "Peak Q", "",
                                       1.0f, 0.5f, 5.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.5f, 5.0f));

    _highshelf_freq = register_float_parameter("highshelf_freq", "High-shelf frequency", "Hz",
                                               4'000.0f, 1'000.0f, 20'000.0f,
                                               Direction::AUTOMATABLE,
                                               new CubicWarpPreProcessor(1'000.0f, 20'000.0f));
    _highshelf_gain = register_float_parameter("highshelf_gain", "High-shelf Gain", "dB",
                                              0.0f, -24.0f, 24.0f,
                                              Direction::AUTOMATABLE,
                                              new dBToLinPreProcessor(-24.0f, 24.0f));
    _highshelf_q = register_float_parameter("highshelf_q", "High-shelf Q", "",
                                           1.0f, 0.5f, 5.0f,
                                           Direction::AUTOMATABLE,
                                           new FloatParameterPreProcessor(0.5f, 5.0f));

    assert(_lowshelf_freq);
    assert(_lowshelf_gain);
    assert(_lowshelf_q);
    assert(_peak_freq);
    assert(_peak_gain);
    assert(_peak_q);
    assert(_highshelf_freq);
    assert(_highshelf_gain);
    assert(_highshelf_q);
}

ProcessorReturnCode Eq3bandPlugin::init(float sample_rate)
{
    bw_ls2_init(&_ls2_coeffs);
    bw_peak_init(&_peak_coeffs);
    bw_hs2_init(&_hs2_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void Eq3bandPlugin::configure(float sample_rate)
{
    bw_ls2_set_sample_rate(&_ls2_coeffs, sample_rate);
    bw_peak_set_sample_rate(&_peak_coeffs, sample_rate);
    bw_hs2_set_sample_rate(&_hs2_coeffs, sample_rate);
    return;
}

void Eq3bandPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_ls2_reset_coeffs(&_ls2_coeffs);
    bw_peak_reset_coeffs(&_peak_coeffs);
    bw_hs2_reset_coeffs(&_hs2_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_ls2_reset_state(&_ls2_coeffs, &_ls2_states[i], 0.0f);
        bw_peak_reset_state(&_peak_coeffs, &_peak_states[i], 0.0f);
        bw_hs2_reset_state(&_hs2_coeffs, &_hs2_states[i], 0.0f);
    }
}

void Eq3bandPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(std::make_unique<SetProcessorBypassEvent>(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void Eq3bandPlugin::process_event(const RtEvent& event)
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

void Eq3bandPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_ls2_set_cutoff(&_ls2_coeffs, _lowshelf_freq->processed_value());
    bw_ls2_set_dc_gain_lin(&_ls2_coeffs, _lowshelf_gain->processed_value());
    bw_ls2_set_Q(&_ls2_coeffs, _lowshelf_q->processed_value());

    bw_peak_set_cutoff(&_peak_coeffs, _peak_freq->processed_value());
    bw_peak_set_peak_gain_lin(&_peak_coeffs, _peak_gain->processed_value());
    bw_peak_set_bandwidth(&_peak_coeffs, _peak_q->processed_value());

    bw_hs2_set_cutoff(&_hs2_coeffs, _highshelf_freq->processed_value());
    bw_hs2_set_high_gain_lin(&_hs2_coeffs, _highshelf_gain->processed_value());
    bw_hs2_set_Q(&_hs2_coeffs, _highshelf_q->processed_value());

    if (_bypass_manager.should_process())
    {
        const float* in_channel_ptrs[_current_input_channels];
        float* out_channel_ptrs[_current_input_channels];
        for (int i = 0; i < _current_input_channels; i++)
        {
            in_channel_ptrs[i] = in_buffer.channel(i);
            out_channel_ptrs[i] = out_buffer.channel(i);
        }

        bw_ls2_update_coeffs_ctrl(&_ls2_coeffs);
        bw_peak_update_coeffs_ctrl(&_peak_coeffs);
        bw_hs2_update_coeffs_ctrl(&_hs2_coeffs);
        for (int n = 0; n < AUDIO_CHUNK_SIZE; n++)
        {
            bw_ls2_update_coeffs_audio(&_ls2_coeffs);
            bw_peak_update_coeffs_audio(&_peak_coeffs);
            bw_hs2_update_coeffs_audio(&_hs2_coeffs);
            for (int i = 0; i < _current_input_channels; i++)
            {
                float x = bw_ls2_process1(&_ls2_coeffs, &_ls2_states[i],
                                          *in_channel_ptrs[i]++);
                float y = bw_peak_process1(&_peak_coeffs, &_peak_states[i], x);
                *out_channel_ptrs[i]++ = bw_hs2_process1(&_hs2_coeffs, &_hs2_states[i], y);
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

std::string_view Eq3bandPlugin::static_uid()
{
    return PLUGIN_UID;
}


} // namespace sushi::internal::eq3band_plugin

