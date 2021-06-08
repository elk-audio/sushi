/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Audio processor with event output example
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "sample_delay_plugin.h"

namespace sushi {
namespace sample_delay_plugin {

SampleDelayPlugin::SampleDelayPlugin(HostControl host_control) : InternalPlugin(host_control),
                                                                 _write_idx(0),
                                                                 _read_idx(0)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _sample_delay = register_int_parameter("sample_delay", 
                                           "Sample delay", 
                                           "samples", 
                                           0,
                                           0,
                                           MAX_DELAY - 1);
    for (int i = 0; i < _current_output_channels; i++)
    {
        _delaylines.push_back(std::make_unique<std::array<float, MAX_DELAY>>());
    }
}

void SampleDelayPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    // update delay value
    _read_idx = (_write_idx + MAX_DELAY - _sample_delay->domain_value()) % MAX_DELAY;

    // process
    if (_bypassed == false)
    {
        int n_channels = std::min(in_buffer.channel_count(), out_buffer.channel_count());
        for (int sample_idx = 0; sample_idx < AUDIO_CHUNK_SIZE; sample_idx++)
        {
            for (int channel_idx = 0; channel_idx < n_channels; channel_idx++)
            {
                _delaylines[channel_idx]->at(_write_idx) = in_buffer.channel(channel_idx)[sample_idx];
                out_buffer.channel(channel_idx)[sample_idx] = _delaylines[channel_idx]->at(_read_idx);
            }
            _write_idx++;
            _read_idx++;
            _write_idx %= MAX_DELAY;
            _read_idx %= MAX_DELAY;
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}


    
} // namespace sample_delay_plugin
} // namespace sushi
