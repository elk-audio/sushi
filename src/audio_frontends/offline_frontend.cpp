#include <cmath>
#include <iostream>

#include "logging.h"
#include "offline_frontend.h"

namespace sushi {
namespace audio_frontend {

MIND_GET_LOGGER;

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
    if (_soundfile_info.channels != _engine->n_channels_in_track(0))
    {
        MIND_LOG_ERROR("Mismatch in number of channels of audio file, which is {}", _soundfile_info.channels);
        cleanup();
        return AudioFrontendStatus::INVALID_N_CHANNELS;
    }
    _buffer = ChunkSampleBuffer(_soundfile_info.channels);
    _engine->set_audio_input_channels(MAX_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(MAX_FRONTEND_CHANNELS);
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

    // Initialize buffers
    _file_buffer = new float[_engine->n_channels_in_track(0) * AUDIO_CHUNK_SIZE];

    return ret_code;
}

void OfflineFrontend::add_sequencer_events_from_json_def(const rapidjson::Document& config)
{
    _event_queue.reserve(config["events"].GetArray().Size());
    for(const auto& e : config["events"].GetArray())
    {
        int sample = static_cast<int>( std::round(e["time"].GetDouble() * static_cast<double>(_engine->sample_rate()) ) );
        const rapidjson::Value& data = e["data"];
        ObjectId processor_id;
        sushi::engine::EngineReturnStatus status;
        std::tie(status, processor_id) = _engine->processor_id_from_name(data["plugin_name"].GetString());
        if (status != sushi::engine::EngineReturnStatus::OK)
        {
            MIND_LOG_WARNING("Unknown plugin name: \"{}\"", data["plugin_name"].GetString());
            continue;
        }
        if (e["type"] == "parameter_change")
        {
            ObjectId parameterId;
            std::tie(status, parameterId) = _engine->parameter_id_from_name(data["plugin_name"].GetString(),
                                                                            data["parameter_name"].GetString());
            if (status != sushi::engine::EngineReturnStatus::OK)
            {
                MIND_LOG_WARNING("Unknown parameter name: {}", data["parameter_name"].GetString());
                continue;
            }
            _event_queue.push_back(std::make_tuple(sample,
                                                   RtEvent::make_parameter_change_event(processor_id,
                                                                                      sample % AUDIO_CHUNK_SIZE,
                                                                                      parameterId,
                                                                                      data["value"].GetFloat())));
        }
        else if (e["type"] == "note_on")
        {
            _event_queue.push_back(std::make_tuple(sample,
                                                   RtEvent::make_note_on_event(processor_id,
                                                                             sample % AUDIO_CHUNK_SIZE,
                                                                             data["note"].GetInt(),
                                                                             data["velocity"].GetFloat())));
        }
        else if (e["type"] == "note_off")
        {
            _event_queue.push_back(std::make_tuple(sample,
                                                   RtEvent::make_note_off_event(processor_id,
                                                                              sample % AUDIO_CHUNK_SIZE,
                                                                              data["note"].GetInt(),
                                                                              data["velocity"].GetFloat())));
        }
    }

    // Sort events by reverse time (lambda function compares first tuple element)
    std::sort(std::begin(_event_queue), std::end(_event_queue),
              [](std::tuple<int, RtEvent> const &t1,
                 std::tuple<int, RtEvent> const &t2)
              {
                  return std::get<0>(t1) >= std::get<0>(t2);
              }
    );
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
    if (_file_buffer)
    {
        delete _file_buffer;
        _file_buffer = nullptr;
    }
}

void OfflineFrontend::run()
{
    int readcount;
    int samplecount = 0;
    float usec_time = 0.0f;
    Time start_time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
    while ( (readcount = static_cast<int>(sf_readf_float(_input_file,
                                                         _file_buffer,
                                                         static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)))) )
    {
        // Update time and sample counter
        _engine->update_time(start_time + std::chrono::microseconds(static_cast<uint64_t>(usec_time)), samplecount);
        samplecount += readcount;
        usec_time += readcount * 1000000.f / _engine->sample_rate();

        // Process all events until the end of the frame
        while ( !_event_queue.empty() && (std::get<0>(_event_queue.back()) < samplecount) )
        {
            auto next_event = _event_queue.back();
            _engine->send_rt_event(std::get<1>(next_event));

            _event_queue.pop_back();
        }

        // Render audio buffer
        _buffer.from_interleaved(_file_buffer);
        _engine->process_chunk(&_buffer, &_buffer);
        _buffer.to_interleaved(_file_buffer);

        // Write to file
        // Should we check the number of samples effectively written?
        // Not done in libsndfile's example
        sf_writef_float(_output_file, _file_buffer, static_cast<sf_count_t>(readcount));
    }
}


}; // end namespace audio_frontend

}; // end namespace sushi