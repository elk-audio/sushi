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
 * @brief Audio processor with event output example
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "sample_delay_plugin.h"

namespace sushi {
namespace sample_delay_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.sample_delay";
constexpr auto DEFAULT_LABEL = "Sample delay";

SampleDelayPlugin::SampleDelayPlugin(HostControl host_control) : InternalPlugin(host_control),
                                                                 _write_idx(0),
                                                                 _read_idx(0)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _sample_delay = register_int_parameter("sample_delay", 
                                           "Sample delay", 
                                           "samples", 
                                           0,
                                           0,
                                           MAX_DELAY - 1,
                                           Direction::AUTOMATABLE);
    for (int i = 0; i < DEFAULT_CHANNELS; i++)
    {
        _delaylines.push_back(std::array<float, MAX_DELAY>());
    }
}

void SampleDelayPlugin::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    _channel_config(channels);
}

void SampleDelayPlugin::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    _channel_config(channels);
}

void SampleDelayPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    // update delay value
    _read_idx = (_write_idx + MAX_DELAY - _sample_delay->processed_value()) % MAX_DELAY;

    // process
    if (_bypassed == false)
    {
        int n_channels = std::min(in_buffer.channel_count(), out_buffer.channel_count());
        for (int channel_idx = 0; channel_idx < n_channels; channel_idx++)
        {
            int temp_write_idx = _write_idx;
            int temp_read_idx = _read_idx;
            for (int sample_idx = 0; sample_idx < AUDIO_CHUNK_SIZE; sample_idx++)
            {
                _delaylines[channel_idx][temp_write_idx] = in_buffer.channel(channel_idx)[sample_idx];
                out_buffer.channel(channel_idx)[sample_idx] = _delaylines[channel_idx][temp_read_idx];
                temp_write_idx++;
                temp_read_idx++;
                temp_write_idx %= MAX_DELAY;
                temp_read_idx %= MAX_DELAY;
            }
        }
        _write_idx += AUDIO_CHUNK_SIZE;
        _read_idx += AUDIO_CHUNK_SIZE;
        _write_idx %= MAX_DELAY;
        _read_idx %= MAX_DELAY;
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

void SampleDelayPlugin::_channel_config(int channels)
{
    int max_channels = std::max(std::max(channels, _current_input_channels), _current_output_channels);
    if (_delaylines.size() != static_cast<size_t>(max_channels))
    {
        _delaylines.resize(max_channels);
        _reset();
    }
}

void SampleDelayPlugin::set_enabled(bool enabled)
{
    if (enabled == false)
    {
        _reset();
    }
}

std::string_view SampleDelayPlugin::static_uid()
{
    return PLUGIN_UID;
}

void SampleDelayPlugin::_reset()
{
    for (auto& line : _delaylines)
    {
        line.fill(0.0f);
    }
    _read_idx = 0;
    _write_idx = 0;
}


} // namespace sample_delay_plugin
} // namespace sushi
