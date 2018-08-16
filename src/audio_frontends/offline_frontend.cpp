#include <cmath>
#include <cstring>

#include "logging.h"
#include "offline_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi {
namespace audio_frontend {

MIND_GET_LOGGER_WITH_MODULE_NAME("offline audio");

AudioFrontendStatus OfflineFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto off_config = static_cast<OfflineFrontendConfiguration*>(_config);

    // Open audio file and check channels / sample rate
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    if (! (_input_file = sf_open(off_config->input_filename.c_str(), SFM_READ, &_soundfile_info)) )
    {
        cleanup();
        MIND_LOG_ERROR("Unable to open input file {}", off_config->input_filename);
        return AudioFrontendStatus::INVALID_INPUT_FILE;
    }
    _mono = _soundfile_info.channels == 1;
    _engine->set_audio_input_channels(OFFLINE_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(OFFLINE_FRONTEND_CHANNELS);
    _engine->set_output_latency(std::chrono::microseconds(0));
    auto sample_rate_file = _soundfile_info.samplerate;
    if (sample_rate_file != _engine->sample_rate())
    {
        MIND_LOG_WARNING("Sample rate mismatch between file ({}) and engine ({})",
                         sample_rate_file,
                         _engine->sample_rate());
    }

    // Open output file with same format as input file
    if (! (_output_file = sf_open(off_config->output_filename.c_str(), SFM_WRITE, &_soundfile_info)) )
    {
        cleanup();
        MIND_LOG_ERROR("Unable to open output file {}", off_config->output_filename);
        return AudioFrontendStatus::INVALID_OUTPUT_FILE;
    }
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
        // Update time and sample counter
        _engine->update_time(start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time)), samplecount);

        samplecount += readcount;
        usec_time += readcount * 1'000'000.f / _engine->sample_rate();

        // Process all events until the end of the frame
        Time chunk_end_time = start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time));
        while (!_event_queue.empty() && _event_queue.back()->time() < chunk_end_time)
        {
            auto next_event = _event_queue.back();
            if (next_event->maps_to_rt_event())
            {
                int offset = time_to_sample_offset(chunk_end_time, next_event->time(), _engine->sample_rate());
                auto rt_event = next_event->to_rt_event(offset);
                _engine->send_rt_event(rt_event);
            }
            _event_queue.pop_back();
            delete next_event;
        }
        _buffer.clear();

        // Render audio buffer
        if (_mono)
        {
            std::copy(file_buffer, file_buffer + AUDIO_CHUNK_SIZE, _buffer.channel(0));
        }
        else
        {
            _buffer.from_interleaved(file_buffer);
        }
        _engine->process_chunk(&_buffer, &_buffer);

        if (_mono)
        {
            std::copy(_buffer.channel(0), _buffer.channel(0) + AUDIO_CHUNK_SIZE, file_buffer );
        }
        else
        {
            _buffer.to_interleaved(file_buffer);
        }

        // Write to file
        // Should we check the number of samples effectively written?
        // Not done in libsndfile's example
        sf_writef_float(_output_file, file_buffer, static_cast<sf_count_t>(readcount));
    }
}


}; // end namespace audio_frontend

}; // end namespace sushi