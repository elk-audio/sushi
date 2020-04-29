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
constexpr int RINGBUFFER_SIZE = 65536;
constexpr int FLUSH_FREQUENCY = (RINGBUFFER_SIZE / 4);
constexpr float DEFAULT_WRITE_INTERVAL = 1.0f;
constexpr float MAX_WRITE_INTERVAL = 4.0f;
constexpr float MIN_WRITE_INTERVAL = 0.5f;

namespace WavWriterStatus {
enum WavWriter : int
{
    SUCCESS = 0,
    FAILURE
};}

class WavWriterPlugin : public InternalPlugin
{
public:
    explicit WavWriterPlugin(HostControl host_control);
    ~WavWriterPlugin();

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    static int non_rt_callback(void* data, EventId id)
    {
        return reinterpret_cast<WavWriterPlugin*>(data)->_non_rt_callback(id);
    }

private:
    void _start_recording();
    void _stop_recording();
    void _post_write_event();
    int _write_to_file();
    int _non_rt_callback(EventId id);

    memory_relaxed_aquire_release::CircularFifo<float, RINGBUFFER_SIZE> _ring_buffer;

    std::vector<float> _file_buffer;
    SNDFILE* _output_file;
    SF_INFO _soundfile_info;

    BoolParameterValue* _recording;
    FloatParameterValue* _write_speed;
    std::string _destination_file_property;

    EventId _pending_event_id{0};
    bool _pending_write_event{false};
    int _flushed_samples_counter{0};
    unsigned int _samples_received{0};

};

} // namespace wav_writer_plugin
} // namespace sushi

#endif // SUSHI_WAVE_WRITER_PLUGIN_H
