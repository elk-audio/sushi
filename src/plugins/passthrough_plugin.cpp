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
 * @brief Example unit gain plugin
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>
#include <cassert>

#include "passthrough_plugin.h"

namespace sushi {
namespace passthrough_plugin {

constexpr auto DEFAULT_NAME = "sushi.testing.passthrough";
constexpr auto DEFAULT_LABEL = "Passthrough";

PassthroughPlugin::PassthroughPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
}

PassthroughPlugin::~PassthroughPlugin() = default;

void PassthroughPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    /* Pass keyboard data/midi through */
    while (!_event_queue.empty())
    {
        RtEvent event;
        if (_event_queue.pop(event))
        {
            output_event(event);
        }
    }
}


}// namespace passthrough_plugin
}// namespace sushi