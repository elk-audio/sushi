#include <logging.h>
#include "offline_frontend.h"

#include <cmath>
#include <iostream>

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
    if (_soundfile_info.channels != _engine->n_channels())
    {
        MIND_LOG_ERROR("Mismatch in number of channels of audio file, which is {}", _soundfile_info.channels);
        cleanup();
        return AudioFrontendStatus::INVALID_N_CHANNELS;
    }
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
    _file_buffer = new float[_engine->n_channels() * AUDIO_CHUNK_SIZE];

    return ret_code;
}

AudioFrontendStatus OfflineFrontend::add_sequencer_events_from_json_def(const Json::Value &events)
{
    if (events.isArray())
    {
        _event_queue.reserve(events.size());
        for(const Json::Value& e : events)
        {
            int sample = static_cast<int>( std::round(e["time"].asDouble() * static_cast<double>(_engine->sample_rate()) ) );
            auto data = e["data"];
            if (e["type"] == "parameter_change")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new ParameterChangeEvent(EventType::FLOAT_PARAMETER_CHANGE,
                                                                                data["stompbox_instance"].asString(),
                                                                                sample % AUDIO_CHUNK_SIZE,
                                                                                data["parameter_id"].asString(),
                                                                                data["value"].asFloat())));

            }
            else if (e["type"] == "note_on")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new KeyboardEvent(EventType::NOTE_ON,
                                                                        data["stompbox_instance"].asString(),
                                                                        sample % AUDIO_CHUNK_SIZE,
                                                                        data["note"].asInt(),
                                                                        data["velocity"].asFloat())));
            }
            else if (e["type"] == "note_off")
            {
                _event_queue.push_back(std::make_tuple(sample,
                                                       new KeyboardEvent(EventType::NOTE_OFF,
                                                                         data["stompbox_instance"].asString(),
                                                                         sample % AUDIO_CHUNK_SIZE,
                                                                         data["note"].asInt(),
                                                                         data["velocity"].asFloat())));
            }
        }

        // Sort events by reverse time (lambda function compares first tuple element)
        std::sort(std::begin(_event_queue), std::end(_event_queue),
                  [](std::tuple<int, BaseEvent*> const &t1,
                     std::tuple<int, BaseEvent*> const &t2)
                  {
                      return std::get<0>(t1) >= std::get<0>(t2);
                  }
        );
    }
    else
    {
        MIND_LOG_ERROR("Invalid format for events in configuration file");
        return AudioFrontendStatus ::INVALID_SEQUENCER_DATA;
    }

    return AudioFrontendStatus ::OK;
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
    while ( (readcount = static_cast<int>(sf_readf_float(_input_file,
                                                         _file_buffer,
                                                         static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)))) )
    {
        samplecount += readcount;
        // Process all events until the end of the frame
        while ( !_event_queue.empty() && (std::get<0>(_event_queue.back()) < samplecount) )
        {
            auto next_event = _event_queue.back();
            auto plugin_event = std::get<1>(next_event);
            _engine->send_rt_event(std::get<1>(next_event));

            _event_queue.pop_back();
            // TODO don't delete things in the audio thread.
            delete plugin_event;
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
