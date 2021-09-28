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

#ifndef SUSHI_SAMPLE_DELAY_PLUGIN_H
#define SUSHI_SAMPLE_DELAY_PLUGIN_H

#include <array>
#include <memory>
#include <vector>

#include "library/internal_plugin.h"

namespace sushi {
namespace sample_delay_plugin {

constexpr int MAX_DELAY = 48000;

class SampleDelayPlugin : public InternalPlugin
{
public:
    SampleDelayPlugin(HostControl host_control);

    virtual ~SampleDelayPlugin() = default;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    // Input parameters
    IntParameterValue* _sample_delay;
    
    // Delayline data
    int _write_idx;
    int _read_idx;
    std::vector<std::array<float, MAX_DELAY>> _delaylines;
};

} // namespace sample_delay_plugin
} // namespace sushi


#endif // !SUSHI_SAMPLE_DELAY_PLUGIN_H
