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
 * @brief Simple 8-step sequencer example.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_WAVE_WRITER_PLUGIN_H
#define SUSHI_WAVE_WRITER_PLUGIN_H

#include <memory>
#include <thread>
#include <atomic>
#include <array>

#include <sndfile.h>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"

#include "library/internal_plugin.h"

namespace sushi {
namespace wav_writer_plugin {

    constexpr int N_AUDIO_CHANNELS = 2;
    constexpr int RINGBUFFER_SIZE = 16384;
    constexpr auto WORKER_SLEEP_INTERVAL = std::chrono::milliseconds(3);

class WavWriterPlugin : public InternalPlugin
{
public:
    explicit WavWriterPlugin(HostControl host_control);
    ~WavWriterPlugin();

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

private:
    void _start_recording();
    void _stop_recording();
    void _file_writer_task();

    char _program_name[32];

    std::thread _worker;
    std::atomic<bool> _worker_running {false};
    memory_relaxed_aquire_release::CircularFifo<float, RINGBUFFER_SIZE> _ring_buffer;

    std::array<float, RINGBUFFER_SIZE> _file_buffer;
    SNDFILE* _output_file;
    SF_INFO _soundfile_info;

    BoolParameterValue* _recording;
};

} // namespace wav_writer_plugin
} // namespace sushi

#endif // SUSHI_WAVE_WRITER_PLUGIN_H
