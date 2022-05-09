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
 * @brief Simple plugin for writing wav files.
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_WAVE_WRITER_PLUGIN_H
#define SUSHI_WAVE_WRITER_PLUGIN_H

#include <sndfile.h>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"

#include "library/internal_plugin.h"

namespace sushi {
namespace wav_writer_plugin {

constexpr int N_AUDIO_CHANNELS = 2;
constexpr int RINGBUFFER_SIZE = 65536 / AUDIO_CHUNK_SIZE;
constexpr int POST_WRITE_FREQUENCY = (RINGBUFFER_SIZE / 4);
constexpr int SAMPLE_WRITE_LIMIT = 48000 * N_AUDIO_CHANNELS * 3600; // Limit file size to 1 hour of stereo audio
constexpr float DEFAULT_WRITE_INTERVAL = 1.0f;
constexpr float MAX_WRITE_INTERVAL = 4.0f;
constexpr float MIN_WRITE_INTERVAL = 0.5f;

enum WavWriterStatus : int
{
    SUCCESS = 0,
    FAILURE
};

class WavWriterPlugin : public InternalPlugin, public UidHelper<WavWriterPlugin>
{
public:
    explicit WavWriterPlugin(HostControl host_control);

    ~WavWriterPlugin();

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    static int non_rt_callback(void* data, EventId id)
    {
        return reinterpret_cast<WavWriterPlugin*>(data)->_non_rt_callback(id);
    }

    static std::string_view static_uid();

private:
    WavWriterStatus _start_recording();
    WavWriterStatus _stop_recording();
    void _post_write_event();
    int _write_to_file();
    int _non_rt_callback(EventId id);
    std::string _available_path(const std::string& requested_path);

    memory_relaxed_aquire_release::CircularFifo<std::array<float, AUDIO_CHUNK_SIZE * N_AUDIO_CHANNELS>, RINGBUFFER_SIZE> _ring_buffer;

    std::vector<float> _file_buffer;
    SNDFILE* _output_file{nullptr};
    SF_INFO _soundfile_info;

    BoolParameterValue* _recording_parameter;
    FloatParameterValue* _write_speed_parameter;
    std::string _actual_file_path;

    float _write_speed{0.0f};

    int _post_write_timer{0};
    unsigned int _samples_received{0};
    sf_count_t _samples_written{0};
    sf_count_t _total_samples_written{0};
};

} // namespace wav_writer_plugin
} // namespace sushi

#endif // SUSHI_WAVE_WRITER_PLUGIN_H
