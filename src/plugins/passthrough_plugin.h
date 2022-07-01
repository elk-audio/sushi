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
 * @brief Example unit gain plugin. Passes audio unprocessed from input to output
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef PASSTHROUGH_PLUGIN_H
#define PASSTHROUGH_PLUGIN_H

#include "library/internal_plugin.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace passthrough_plugin {

class PassthroughPlugin : public InternalPlugin, public UidHelper<PassthroughPlugin>
{
public:
    explicit PassthroughPlugin(HostControl host_control);

    ~PassthroughPlugin() override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
};

}// namespace passthrough_plugin
}// namespace sushi
#endif //PASSTHROUGH_PLUGIN_H
