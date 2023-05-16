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
 * @brief Bitcrusher from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include "bitcrusher_plugin.h"

namespace sushi {
namespace bitcrusher_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.bitcrusher";
constexpr auto DEFAULT_LABEL = "Bitcrusher";


BitcrusherPlugin::BitcrusherPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_TRACK_CHANNELS;
    _max_output_channels = MAX_TRACK_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _samplerate_ratio = register_float_parameter("sr_ratio", "Samplerate ratio", "",
                                                 1.0f, 0.0f, 1.0f,
                                                 Direction::AUTOMATABLE,
                                                 new FloatParameterPreProcessor(0.0f, 1.0f));
    _bit_depth = register_int_parameter("bit_depth", "Bit Depth", "",
                                        16, 1, 16,
                                        Direction::AUTOMATABLE,
                                        new IntParameterPreProcessor(1, 16));

    assert(_samplerate_ratio);
    assert(_bit_depth);
}

ProcessorReturnCode BitcrusherPlugin::init(float sample_rate)
{
    bw_sr_reduce_init(&_sr_reduce_coeffs);
    bw_bd_reduce_init(&_bd_reduce_coeffs);
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void BitcrusherPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    bw_bd_reduce_reset_coeffs(&_bd_reduce_coeffs);
    for (int i = 0; i < MAX_TRACK_CHANNELS; i++)
    {
        bw_sr_reduce_reset_state(&_sr_reduce_coeffs, &_sr_reduce_states[i]);
    }
}

void BitcrusherPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void BitcrusherPlugin::process_event(const RtEvent& event)
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

void BitcrusherPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* Update parameter values */
    bw_sr_reduce_set_ratio(&_sr_reduce_coeffs, _samplerate_ratio->processed_value());
    bw_bd_reduce_set_bit_depth(&_bd_reduce_coeffs, _bit_depth->processed_value());

    if (_bypass_manager.should_process())
    {
        for (int i = 0; i < _current_input_channels; i++)
        {
            bw_sr_reduce_process(&_sr_reduce_coeffs, &_sr_reduce_states[i],
                                 in_buffer.channel(i), out_buffer.channel(i),
                                 AUDIO_CHUNK_SIZE);
            bw_bd_reduce_process(&_bd_reduce_coeffs,
                                 out_buffer.channel(i), out_buffer.channel(i),
                                 AUDIO_CHUNK_SIZE);
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

std::string_view BitcrusherPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace bitcrusher_plugin
}// namespace sushi

