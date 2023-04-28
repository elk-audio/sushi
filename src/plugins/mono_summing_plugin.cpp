/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "mono_summing_plugin.h"

namespace sushi {
namespace mono_summing_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.mono_summing";
constexpr auto DEFAULT_LABEL = "Mono summing";

MonoSummingPlugin::MonoSummingPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
}

MonoSummingPlugin::~MonoSummingPlugin() = default;

void MonoSummingPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypassed == false)
    {
        for (int output_channel = 0; output_channel < out_buffer.channel_count(); ++output_channel)
        {
            int input_channel = 0;
            out_buffer.replace(output_channel, input_channel, in_buffer);
            for (input_channel = 1; input_channel < in_buffer.channel_count(); ++input_channel)
            {
                out_buffer.add(output_channel, input_channel, in_buffer);
            }
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }

}

std::string_view MonoSummingPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace mono_summing_plugin
}// namespace sushi
