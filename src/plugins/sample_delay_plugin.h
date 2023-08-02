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
 * @brief Audio processor with event output example
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_SAMPLE_DELAY_PLUGIN_H
#define SUSHI_SAMPLE_DELAY_PLUGIN_H

#include <array>
#include <memory>
#include <vector>

#include "library/internal_plugin.h"

namespace sushi::internal::sample_delay_plugin {

constexpr int MAX_DELAY = 48000;

class SampleDelayPlugin : public InternalPlugin, public UidHelper<SampleDelayPlugin>
{
public:
    explicit SampleDelayPlugin(HostControl host_control);

    ~SampleDelayPlugin() override = default;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    void _channel_config(int channels);

    void _reset();

    // Input parameters
    IntParameterValue* _sample_delay;
    
    // Delayline data
    int _write_idx;
    int _read_idx;
    std::vector<std::array<float, MAX_DELAY>> _delaylines;
};

} // end namespace sushi::internal::sample_delay_plugin

#endif // !SUSHI_SAMPLE_DELAY_PLUGIN_H
