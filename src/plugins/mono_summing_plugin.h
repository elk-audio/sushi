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
 * @brief Plugin to sum all input channels and output the result to all output channels
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_MONO_SUMMING_PLUGIN_H
#define SUSHI_MONO_SUMMING_PLUGIN_H

#include "library/internal_plugin.h"
#include "library/rt_event_fifo.h"

ELK_PUSH_WARNING
ELK_DISABLE_DOMINANCE_INHERITANCE

namespace sushi::internal::mono_summing_plugin {

class MonoSummingPlugin : public InternalPlugin, public UidHelper<MonoSummingPlugin>
{
public:
    MonoSummingPlugin(HostControl host_control);

    ~MonoSummingPlugin();

    void process_event(const RtEvent& event) override
    {
        InternalPlugin::process_event(event);
    };

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();
};

} // end namespace sushi::internal::mono_summing_plugin

ELK_POP_WARNING

#endif // SUSHI_MONO_SUMMING_PLUGIN_H
