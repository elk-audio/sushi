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
* @brief Offline frontend to process audio files in chunks
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#include <cmath>
#include <cstring>
#include <random>

#include "logging.h"
#include "offline_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("offline audio");

#if defined(__clang__)
constexpr float INPUT_NOISE_LEVEL = 0.06309573444801933; // pow(10, (-24.0f/20.0f)) pre-computed
#elif defined(__GNUC__) || defined(__GNUG__)
constexpr float INPUT_NOISE_LEVEL = powf(10, (-24.0f/20.0f));
#endif

constexpr int   NOISE_SEED = 5; // Using a constant seed makes potential errors reproducible

template<class random_device, class random_dist>
void fill_buffer_with_noise(ChunkSampleBuffer& buffer, random_device& dev, random_dist& dist)
{
    for (int c = 0; c < buffer.channel_count(); ++c)
    {
        float* channel = buffer.channel(c);
        for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
        {
            channel[i] = dist(dev);
        }
    }
}

template<class random_device, class random_dist>
void fill_cv_buffer_with_noise(engine::ControlBuffer& buffer, random_device& dev, random_dist& dist)
{
    for (auto& cv : buffer.cv_values)
    {
        cv = map_audio_to_cv(dist(dev));
    }
}

AudioFrontendStatus OfflineFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto off_config = static_cast<OfflineFrontendConfiguration*>(_config);
    _dummy_mode = off_config->dummy_mode;

    if (_dummy_mode == false)
    {
        // Open audio file and check channels / sample rate
        memset(&_soundfile_info, 0, sizeof(_soundfile_info));
        if (!(_input_file = sf_open(off_config->input_filename.c_str(), SFM_READ, &_soundfile_info)))
        {
            cleanup();
            SUSHI_LOG_ERROR("Unable to open input file {}", off_config->input_filename);
            return AudioFrontendStatus::INVALID_INPUT_FILE;
        }
        _mono = _soundfile_info.channels == 1;
        auto sample_rate_file = _soundfile_info.samplerate;
        if (sample_rate_file != _engine->sample_rate())
        {
            SUSHI_LOG_WARNING("Sample rate mismatch between file ({}) and engine ({})",
                              sample_rate_file,
                              _engine->sample_rate());
        }

        // Open output file with same format as input file
        if (!(_output_file = sf_open(off_config->output_filename.c_str(), SFM_WRITE, &_soundfile_info)))
        {
            cleanup();
            SUSHI_LOG_ERROR("Unable to open output file {}", off_config->output_filename);
            return AudioFrontendStatus::INVALID_OUTPUT_FILE;
        }
        _engine->set_audio_input_channels(OFFLINE_FRONTEND_CHANNELS);
        _engine->set_audio_output_channels(OFFLINE_FRONTEND_CHANNELS);
    }
    else
    {
        _engine->set_audio_input_channels(DUMMY_FRONTEND_CHANNELS);
        _engine->set_audio_output_channels(DUMMY_FRONTEND_CHANNELS);
    }
    
    auto status = _engine->set_cv_input_channels(off_config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv inputs failed", off_config->cv_inputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    status = _engine->set_cv_output_channels(off_config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv outputs failed", off_config->cv_outputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    _engine->set_output_latency(std::chrono::microseconds(0));

    return ret_code;
}

void OfflineFrontend::add_sequencer_events(std::vector<Event*> events)
{
    // Sort events by reverse time
    std::sort(events.begin(), events.end(), [](const Event* lhs, const Event* rhs)
                                              {
                                                  return lhs->time() >= rhs->time();
                                              });
    _event_queue = std::move(events);
}

void OfflineFrontend::cleanup()
{
    _running = false;
    if (_worker.joinable())
    {
        _worker.join();
    }
    if (_input_file)
    {
        sf_close(_input_file);
        _input_file = nullptr;
    }
    if (_output_file)
    {
        sf_close(_output_file);
        _output_file = nullptr;
    }
}

int time_to_sample_offset(Time chunk_end_time, Time event_time, float samplerate)
{
    Time chunktime = std::chrono::microseconds(static_cast<uint64_t>(1'000'000.f * AUDIO_CHUNK_SIZE / samplerate));
    return AUDIO_CHUNK_SIZE - static_cast<int>((AUDIO_CHUNK_SIZE * (chunk_end_time - event_time) / chunktime));
}

void OfflineFrontend::run()
{
    if (_dummy_mode)
    {
        _worker = std::thread(&OfflineFrontend::_process_dummy, this);
    }
    else
    {
        _run_blocking();
    }
}

// Process all events up until end_time
void OfflineFrontend::_process_events(Time end_time)
{
    while (!_event_queue.empty() && _event_queue.back()->time() < end_time)
    {
        auto next_event = _event_queue.back();
        if (next_event->maps_to_rt_event())
        {
            int offset = time_to_sample_offset(end_time, next_event->time(), _engine->sample_rate());
            auto rt_event = next_event->to_rt_event(offset);
            _engine->send_rt_event(rt_event);
        }
        _event_queue.pop_back();
        delete next_event;
    }
}

void OfflineFrontend::_process_dummy()
{
    set_flush_denormals_to_zero();
    int samplecount = 0;
    double usec_time = 0.0f;
    Time start_time = std::chrono::microseconds(0);

    std::ranlux24 rand_gen;
    rand_gen.seed(NOISE_SEED);
    std::normal_distribution<float> normal_dist(0.0f, INPUT_NOISE_LEVEL);

    while (_running)
    {
        auto process_time = start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time));

        samplecount += AUDIO_CHUNK_SIZE;
        usec_time += AUDIO_CHUNK_SIZE * 1'000'000.f / _engine->sample_rate();

        Time chunk_end_time = start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time));
        _process_events(chunk_end_time);

        fill_buffer_with_noise(_buffer, rand_gen, normal_dist);
        fill_cv_buffer_with_noise(_control_buffer, rand_gen, normal_dist);
        _engine->process_chunk(&_buffer, &_buffer, &_control_buffer, &_control_buffer, process_time, samplecount);
    }
}

void OfflineFrontend::_run_blocking()
{
    set_flush_denormals_to_zero();
    int readcount;
    int samplecount = 0;
    double usec_time = 0.0f;
    Time start_time = std::chrono::microseconds(0);

    float file_buffer[OFFLINE_FRONTEND_CHANNELS * AUDIO_CHUNK_SIZE];
    while ( (readcount = static_cast<int>(sf_readf_float(_input_file,
                                                         file_buffer,
                                                         static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)))) )
    {
        auto process_time = start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time));

        samplecount += readcount;
        usec_time += readcount * 1'000'000.f / _engine->sample_rate();

        Time chunk_end_time = start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time));
        _process_events(chunk_end_time);

        _buffer.clear();

        if (_mono)
        {
            std::copy(file_buffer, file_buffer + AUDIO_CHUNK_SIZE, _buffer.channel(0));
        }
        else
        {
            auto buffer = ChunkSampleBuffer::create_non_owning_buffer(_buffer, 0, 2);
            buffer.from_interleaved(file_buffer);
        }
        /* Gate and CV are ignored when using file frontend */
        _engine->process_chunk(&_buffer, &_buffer, &_control_buffer, &_control_buffer, process_time, samplecount);

        if (_mono)
        {
            std::copy(_buffer.channel(0), _buffer.channel(0) + AUDIO_CHUNK_SIZE, file_buffer );
        }
        else
        {
            auto buffer = ChunkSampleBuffer::create_non_owning_buffer(_buffer, 0, 2);
            buffer.to_interleaved(file_buffer);
        }

        // Write to file
        // Should we check the number of samples effectively written?
        // Not done in libsndfile's example
        sf_writef_float(_output_file, file_buffer, static_cast<sf_count_t>(readcount));
    }
}

void OfflineFrontend::pause(bool /*enabled*/)
{
    // Currently a no-op
}


} // end namespace audio_frontend

} // end namespace sushi