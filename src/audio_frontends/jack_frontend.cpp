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
 * @brief Realtime audio frontend for Jack Audio
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_JACK
#include <jack/midiport.h>

#include "logging.h"
#include "jack_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("jack audio");

AudioFrontendStatus JackFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto jack_config = static_cast<JackFrontendConfiguration*>(_config);
    _autoconnect_ports = jack_config->autoconnect_ports;
    _engine->set_audio_input_channels(MAX_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(MAX_FRONTEND_CHANNELS);
    auto status = _engine->set_cv_input_channels(jack_config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv inputs failed", jack_config->cv_inputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    _no_cv_input_ports = jack_config->cv_inputs;
    status = _engine->set_cv_output_channels(jack_config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv outputs failed", jack_config->cv_outputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    _no_cv_output_ports = jack_config->cv_outputs;
    return setup_client(jack_config->client_name, jack_config->server_name);
}

void JackFrontend::cleanup()
{
    if (_client)
    {
        jack_client_close(_client);
        _client = nullptr;
    }
    _engine->enable_realtime(false);
}

void JackFrontend::run()
{
    _engine->enable_realtime(true);
    int status = jack_activate(_client);
    if (status != 0)
    {
        SUSHI_LOG_ERROR("Failed to activate Jack client, error {}.", status);
    }
    if (_autoconnect_ports)
    {
        connect_ports();
    }
}

AudioFrontendStatus JackFrontend::setup_client(const std::string& client_name,
                                               const std::string& server_name)
{
    jack_status_t jack_status;
    jack_options_t options = JackNullOption;
    if (!server_name.empty())
    {
        SUSHI_LOG_ERROR("Using option JackServerName");
        options = JackServerName;
    }
    _client = jack_client_open(client_name.c_str(), options, &jack_status, server_name.c_str());
    if (_client == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to open Jack server, error: {}.", jack_status);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    /* Set process callback function */
    int ret = jack_set_process_callback(_client, rt_process_callback, this);
    if (ret != 0)
    {
        SUSHI_LOG_ERROR("Failed to set Jack callback function, error: {}.", ret);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    ret = jack_set_latency_callback(_client, latency_callback, this);
    if (ret != 0)
    {
        SUSHI_LOG_ERROR("Failed to set latency callback function, error: {}.", ret);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    auto status = setup_sample_rate();
    if (status != AudioFrontendStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup sample rate handling");
        return status;
    }
    status = setup_ports();
    if (status != AudioFrontendStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup ports");
        return status;
    }
    status = setup_cv_ports();
    if (status != AudioFrontendStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup cv ports");
        return status;
    }
    return AudioFrontendStatus::OK;
}

AudioFrontendStatus JackFrontend::setup_sample_rate()
{
    _sample_rate = jack_get_sample_rate(_client);
    if (std::lround(_sample_rate) != _engine->sample_rate())
    {
        SUSHI_LOG_WARNING("Sample rate mismatch between engine ({}) and jack ({}), setting to {}",
                          _engine->sample_rate(), _sample_rate, _sample_rate);
        _engine->set_sample_rate(_sample_rate);
    }
    auto status = jack_set_sample_rate_callback(_client, samplerate_callback, this);
    if (status != 0)
    {
        SUSHI_LOG_WARNING("Setting sample rate callback failed with error {}", status);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

AudioFrontendStatus JackFrontend::setup_ports()
{
    int port_no = 0;
    for (auto& port : _output_ports)
    {
        port = jack_port_register (_client,
                                   std::string("audio_output_" + std::to_string(port_no++)).c_str(),
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsOutput,
                                   0);
        if (port == nullptr)
        {
            SUSHI_LOG_ERROR("Failed to open Jack output port {}.", port_no - 1);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    port_no = 0;
    for (auto& port : _input_ports)
    {
        port = jack_port_register (_client,
                                   std::string("audio_input_" + std::to_string(port_no++)).c_str(),
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsInput,
                                   0);
        if (port == nullptr)
        {
            SUSHI_LOG_ERROR("Failed to open Jack input port {}.", port_no - 1);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    return AudioFrontendStatus::OK;
}


AudioFrontendStatus JackFrontend::setup_cv_ports()
{
    for (int i = 0; i < _no_cv_input_ports; ++i)
    {
        _cv_input_ports[i] = jack_port_register (_client,
                                                 std::string("cv_input_" + std::to_string(i)).c_str(),
                                                 JACK_DEFAULT_AUDIO_TYPE,
                                                 JackPortIsInput,
                                                 0);
        if (_cv_input_ports[i] == nullptr)
        {
            SUSHI_LOG_ERROR("Failed to open Jack cv input port {}.", i);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    for (int i = 0; i < _no_cv_output_ports; ++i)
    {
        _cv_output_ports[i] = jack_port_register (_client,
                                                  std::string("cv_output_" + std::to_string(i)).c_str(),
                                                  JACK_DEFAULT_AUDIO_TYPE,
                                                  JackPortIsOutput,
                                                  0);
        if (_cv_output_ports[i] == nullptr)
        {
            SUSHI_LOG_ERROR("Failed to open Jack cv output port {}.", i);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    return AudioFrontendStatus::OK;
}

/*
 * Searches for external ports and tries to autoconnect them with sushis ports.
 */
AudioFrontendStatus JackFrontend::connect_ports()
{
    const char** out_ports = jack_get_ports(_client, nullptr, nullptr, JackPortIsPhysical|JackPortIsInput);
    if (out_ports == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to get ports from Jack.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    for (int id = 0; id < static_cast<int>(_input_ports.size()); ++id)
    {
        if (out_ports[id])
        {
            int ret = jack_connect(_client, jack_port_name(_output_ports[id]), out_ports[id]);
            if (ret != 0)
            {
                SUSHI_LOG_WARNING("Failed to connect out port {}, error {}.", jack_port_name(_output_ports[id]), id);
            }
        }
    }
    jack_free(out_ports);

    /* Same for input ports */
    const char** in_ports = jack_get_ports(_client, nullptr, nullptr, JackPortIsPhysical|JackPortIsOutput);
    if (in_ports == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to get ports from Jack.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    for (int id = 0; id < static_cast<int>(_input_ports.size()); ++id)
    {
        if (in_ports[id])
        {
            int ret = jack_connect(_client, jack_port_name(_input_ports[id]), in_ports[id]);
            if (ret != 0)
            {
                SUSHI_LOG_WARNING("Failed to connect port {}, error {}.", jack_port_name(_input_ports[id]), id);
            }
        }
    }
    jack_free(in_ports);

    return AudioFrontendStatus::OK;
}


int JackFrontend::internal_process_callback(jack_nframes_t framecount)
{
    set_flush_denormals_to_zero();
    if (framecount < 64 || framecount % 64)
    {
        SUSHI_LOG_CRITICAL("Chunk size not a multiple of AUDIO_CHUNK_SIZE. Skipping.");
        return 0;
    }
    jack_nframes_t 	current_frames{0};
    jack_time_t 	current_usecs{0};
    jack_time_t 	next_usecs{0};
    float           period_usec{0.0};
    if (jack_get_cycle_times(_client, &current_frames, &current_usecs, &next_usecs, &period_usec) > 0)
    {
        SUSHI_LOG_ERROR("Error getting time from jack frontend");
    }
    if (_start_frame == 0 && current_frames > 0)
    {
        _start_frame = current_frames;
    }

    /* Process in chunks of AUDIO_CHUNK_SIZE */
    Time start_time = std::chrono::microseconds(current_usecs);
    for (jack_nframes_t frame = 0; frame < framecount; frame += AUDIO_CHUNK_SIZE)
    {
        Time delta_time = std::chrono::microseconds((frame * 1'000'000) / _sample_rate);
        process_audio(frame, AUDIO_CHUNK_SIZE, start_time + delta_time, current_frames + frame - _start_frame);
    }

    if (_pause_notified == false && _pause_manager.should_process() == false)
    {
        _pause_notify->notify();
        _pause_notified = true;
    }
    return 0;
}

int JackFrontend::internal_samplerate_callback(jack_nframes_t sample_rate)
{
    /* It's not fully clear if this is needed since the sample rate can't
     * change without restarting the Jack server. Thought is's hinted that
     * this could be called with a different sample rated than the one
     * requested if the interface doesn't support it. */
    if (_sample_rate != sample_rate)
    {
        SUSHI_LOG_DEBUG("Received a sample rate change from Jack ({})", sample_rate);
        _engine->set_sample_rate(sample_rate);
        _sample_rate = sample_rate;
    }
    return 0;
}

void JackFrontend::internal_latency_callback(jack_latency_callback_mode_t mode)
{
    /* Currently, all we want to know is the output latency to a physical
     * audio output.
     * We also don't support individual latency compensation on ports, so
     * we get the maximum latency and pass that on to Sushi. */
    if (mode == JackPlaybackLatency)
    {
        int sample_latency = 0;
        jack_latency_range_t range;
        for (auto& port : _output_ports)
        {
            jack_port_get_latency_range(port, JackPlaybackLatency, &range);
            sample_latency = std::max(sample_latency, static_cast<int>(range.max));
        }
        Time latency = std::chrono::microseconds((sample_latency * 1'000'000) / _sample_rate);
        _engine->set_output_latency(latency);
        SUSHI_LOG_INFO("Updated output latency: {} samples, {} ms", sample_latency, latency.count() / 1000.0f);
    }
}

void inline JackFrontend::process_audio(jack_nframes_t start_frame, jack_nframes_t framecount,
                                        Time timestamp, int64_t samplecount)
{
    /* Copy jack buffer data to internal buffers */
    for (size_t i = 0; i < _input_ports.size(); ++i)
    {
        float* in_data = static_cast<float*>(jack_port_get_buffer(_input_ports[i], framecount)) + start_frame;
        std::copy(in_data, in_data + AUDIO_CHUNK_SIZE, _in_buffer.channel(i));
    }
    for (int i = 0; i < _no_cv_input_ports; ++i)
    {
        float* in_data = static_cast<float*>(jack_port_get_buffer(_cv_input_ports[i], framecount)) + start_frame;
        _in_controls.cv_values[i] = map_audio_to_cv(in_data[AUDIO_CHUNK_SIZE - 1]);
    }

    _out_buffer.clear();

    if (_pause_manager.should_process())
    {
        _engine->process_chunk(&_in_buffer, &_out_buffer, &_in_controls, &_out_controls, timestamp, samplecount);
        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(_out_buffer);
        }
    }

    for (size_t i = 0; i < _input_ports.size(); ++i)
    {
        float* out_data = static_cast<float*>(jack_port_get_buffer(_output_ports[i], framecount)) + start_frame;
        std::copy(_out_buffer.channel(i), _out_buffer.channel(i) + AUDIO_CHUNK_SIZE, out_data);
    }
    /* The jack frontend both inputs and outputs cv in audio range [-1, 1] */
    for (int i = 0; i < _no_cv_output_ports; ++i)
    {
        float* out_data = static_cast<float*>(jack_port_get_buffer(_cv_output_ports[i], framecount)) + start_frame;
        _cv_output_hist[i] = ramp_cv_output(out_data, _cv_output_hist[i], map_cv_to_audio(_out_controls.cv_values[i]));
    }
}

} // end namespace audio_frontend
} // end namespace sushi
#endif
#ifndef SUSHI_BUILD_WITH_JACK
#include "audio_frontends/jack_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
SUSHI_GET_LOGGER;
JackFrontend::JackFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{}
AudioFrontendStatus JackFrontend::init(BaseAudioFrontendConfiguration*)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    SUSHI_LOG_ERROR("Sushi was not built with Jack support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}}}
#endif
