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
 * @brief Plugin to sum all input channels and output the result to all output channels
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>
#include <cassert>

#include "mono_summing_plugin.h"

namespace sushi {
namespace mono_summing_plugin {

MonoSummingPlugin::MonoSummingPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
}

MonoSummingPlugin::~MonoSummingPlugin()
{}

void MonoSummingPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if(_bypassed == false)
    {
        ChunkSampleBuffer result(1);
        result.clear();
        for (int channel = 0; channel < input_channels(); ++channel)
        {
            for (int sample = 0; sample < AUDIO_CHUNK_SIZE; ++sample)
            {
                result.channel(0)[sample] += in_buffer.channel(channel)[sample];
            }
        }
        out_buffer.replace(result);
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }

}


}// namespace mono_summing_plugin
}// namespace sushi
