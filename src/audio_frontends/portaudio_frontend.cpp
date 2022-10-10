/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Realtime audio frontend for PortAudio
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_PORTAUDIO

#include "logging.h"
#include "portaudio_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("portaudio");

AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto portaudio_config = static_cast<PortAudioFrontendConfiguration*>(config);

    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    ret_code = _initialize_portaudio();
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    // Setup devices
    int device_count = Pa_GetDeviceCount();
    int input_device_id = portaudio_config->input_device_id.value_or(Pa_GetDefaultInputDevice());
    if (input_device_id >= device_count)
    {
        SUSHI_LOG_ERROR("Input device id is out of range");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    // If there is no available input device the default device will
    // return negative which will cause issues later
    else if (input_device_id < 0)
    {
        input_device_id = 0;
    }

    int output_device_id = portaudio_config->output_device_id.value_or(Pa_GetDefaultOutputDevice());
    if (output_device_id >= device_count)
    {
        SUSHI_LOG_ERROR("Output device id is out of range");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    // If there is no available output device the default device will
    // return negative which will cause issues later
    else if (output_device_id < 0)
    {
        output_device_id = 0;
    }

    _input_device_info = Pa_GetDeviceInfo(input_device_id);
    _output_device_info = Pa_GetDeviceInfo(output_device_id);

    // Setup audio and CV channels
    auto channel_conf_result = _configure_audio_channels(portaudio_config);
    if (channel_conf_result != AudioFrontendStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to configure audio channels");
        return channel_conf_result;
    }
    SUSHI_LOG_DEBUG("Setting up port audio with {} inputs {} outputs", _num_total_input_channels, _num_total_output_channels);

    // Setup device parameters
    PaStreamParameters input_parameters;
    bzero(&input_parameters, sizeof(input_parameters));
    input_parameters.device = input_device_id;
    input_parameters.channelCount = _audio_input_channels + _cv_input_channels;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = portaudio_config->suggested_input_latency;
    input_parameters.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters output_parameters;
    bzero(&output_parameters, sizeof(output_parameters));
    output_parameters.device = output_device_id;
    output_parameters.channelCount = _audio_output_channels + _cv_output_channels;
    output_parameters.sampleFormat = paFloat32;
    output_parameters.suggestedLatency = portaudio_config->suggested_output_latency;
    output_parameters.hostApiSpecificStreamInfo = nullptr;
    // Setup samplerate
    double samplerate = _engine->sample_rate();
    if (_configure_samplerate(&input_parameters, &output_parameters, &samplerate) == false)
    {
        SUSHI_LOG_ERROR("Failed to configure samplerate");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    if (samplerate != _engine->sample_rate())
    {
        SUSHI_LOG_WARNING("Failed to use engine samplerate ({}), using {} instead", _engine->sample_rate(), samplerate);
        _engine->set_sample_rate(samplerate);
    }

    // Open the stream
    // In case there is no input device available we only want to use output
    auto input_param_ptr = (_audio_input_channels + _cv_input_channels) > 0 ? &input_parameters : NULL;
    PaError err = Pa_OpenStream(&_stream,
                                input_param_ptr,
                                &output_parameters,
                                samplerate,
                                AUDIO_CHUNK_SIZE,
                                paNoFlag,
                                &rt_process_callback,
                                static_cast<void*>(this));
    if (err != paNoError)
    {
        SUSHI_LOG_ERROR("Failed to open stream: {}", Pa_GetErrorText(err));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    else
    {
        _stream_initialized = true;
    }
    auto stream_info = Pa_GetStreamInfo(_stream);
    Time latency = std::chrono::microseconds(static_cast<int>(stream_info->outputLatency * 1'000'000));
    _engine->set_output_latency(latency);

    _time_offset = Pa_GetStreamTime(_stream);
    _start_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());

    if (_audio_input_channels + _cv_input_channels > 0)
    {
        SUSHI_LOG_INFO("Connected input channels to {}", _input_device_info->name);
        SUSHI_LOG_INFO("Input device has {} available channels", _input_device_info->maxInputChannels);
    }
    else
    {
        SUSHI_LOG_DEBUG("No input channels found not connecting to input device");
    }

    if (_audio_output_channels + _cv_output_channels > 0)
    {
        SUSHI_LOG_DEBUG("Connected output channels to {}", _output_device_info->name);
    }
    else
    {
        SUSHI_LOG_INFO("No output channels found not connecting to output device");
        SUSHI_LOG_INFO("Output device has {} available channels", _output_device_info->maxOutputChannels);
    }
    SUSHI_LOG_INFO("Stream started, using input latency {} and output latency {}", stream_info->inputLatency, stream_info->outputLatency);

    return AudioFrontendStatus::OK;
}

void PortAudioFrontend::cleanup()
{
    PaError result;
    if (_engine != nullptr)
    {
        _engine->enable_realtime(false);
    }

    if (_stream_initialized)
    {
        result = Pa_IsStreamActive(_stream);
        if (result == 1)
        {
            SUSHI_LOG_INFO("Closing PortAudio stream");
            Pa_StopStream(_stream);
        }
        else if ((result != paNoError) && (_engine != nullptr))
        {
            SUSHI_LOG_WARNING("Error while checking for active stream: {}", Pa_GetErrorText(result));
        }
    }

    result = Pa_Terminate();
    if (result != paNoError)
    {
        SUSHI_LOG_WARNING("Error while terminating Portaudio: {}", Pa_GetErrorText(result));
    }
}

void PortAudioFrontend::run()
{
    _engine->enable_realtime(true);
    Pa_StartStream(_stream);
}

std::optional<int> PortAudioFrontend::devices_count()
{
    if (! (_initialize_portaudio() == AudioFrontendStatus::OK))
    {
        return std::nullopt;
    }

    int devices = Pa_GetDeviceCount();
    if (devices < 0)
    {
        SUSHI_LOG_ERROR("Error querying portaudio devices: {}", devices);
        return std::nullopt;
    }

    return devices;
}

std::optional<PortaudioDeviceInfo> PortAudioFrontend::device_info(int device_idx)
{
    if (! (_initialize_portaudio() == AudioFrontendStatus::OK))
    {
        return std::nullopt;
    }

    const PaDeviceInfo* pa_devinfo = Pa_GetDeviceInfo(device_idx);
    if (pa_devinfo == nullptr)
    {
        SUSHI_LOG_ERROR("Error querying portaudio devices {}", device_idx);
        return std::nullopt;
    }

    PortaudioDeviceInfo devinfo;
    devinfo.name = pa_devinfo->name;
    devinfo.inputs = pa_devinfo->maxInputChannels;
    devinfo.outputs = pa_devinfo->maxOutputChannels;

    return devinfo;
}

std::optional<int> PortAudioFrontend::default_input_device()
{
    if (! (_initialize_portaudio() == AudioFrontendStatus::OK))
    {
        return std::nullopt;
    }

    int default_input = Pa_GetDefaultInputDevice();
    if (default_input == paNoDevice)
    {
        SUSHI_LOG_WARNING("Could not retrieve default input device");
        return std::nullopt;
    }

    return default_input;
}

std::optional<int> PortAudioFrontend::default_output_device()
{
    if (! (_initialize_portaudio() == AudioFrontendStatus::OK))
    {
        return std::nullopt;
    }

    int default_output = Pa_GetDefaultOutputDevice();
    if (default_output == paNoDevice)
    {
        SUSHI_LOG_WARNING("Could not retrieve default output device");
        return std::nullopt;
    }

    return default_output;
}

AudioFrontendStatus PortAudioFrontend::_initialize_portaudio()
{
    if (_pa_initialized)
    {
        return AudioFrontendStatus::OK;
    }

    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        SUSHI_LOG_ERROR("Error initializing PortAudio: {}", Pa_GetErrorText(err));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

AudioFrontendStatus PortAudioFrontend::_configure_audio_channels(const PortAudioFrontendConfiguration* config)
{
    if (_input_device_info == nullptr)
    {
        SUSHI_LOG_ERROR("Configure audio channels called before input device info was collected");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    if (_output_device_info == nullptr)
    {
        SUSHI_LOG_ERROR("Configure audio channels called before output device info was collected");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    _num_total_input_channels = _input_device_info->maxInputChannels;
    _num_total_output_channels = _output_device_info->maxOutputChannels;

    _cv_input_channels = config->cv_inputs;
    _cv_output_channels = config->cv_outputs;
    if (_cv_input_channels > _num_total_input_channels)
    {
        SUSHI_LOG_ERROR("Requested more CV channels than available input channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    if (_cv_output_channels > _num_total_output_channels)
    {
        SUSHI_LOG_ERROR("Requested more CV channels than available output channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _audio_input_channels = _num_total_input_channels - _cv_input_channels;
    _audio_output_channels = _num_total_output_channels - _cv_output_channels;
    _in_buffer = ChunkSampleBuffer(_audio_input_channels);
    _out_buffer = ChunkSampleBuffer(_audio_output_channels);
    _engine->set_audio_input_channels(_audio_input_channels);
    _engine->set_audio_output_channels(_audio_output_channels);
    auto status = _engine->set_cv_input_channels(_cv_input_channels);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup CV input channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    status = _engine->set_cv_output_channels(_cv_output_channels);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup CV output channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

bool PortAudioFrontend::_configure_samplerate(const PaStreamParameters* input_parameters,
                                              const PaStreamParameters* output_parameters,
                                              double* samplerate)
{
    std::array<double, 3> samplerates = {
        *samplerate,
        _input_device_info->defaultSampleRate,
        _output_device_info->defaultSampleRate
    };
    PaError result;
    for (auto sr : samplerates)
    {
        result = Pa_IsFormatSupported(input_parameters, output_parameters, sr);
        if (result != 0)
        {
            SUSHI_LOG_WARNING("Error configuring samplerate {}: {}", sr, Pa_GetErrorText(result));
        }
        else
        {
            *samplerate = sr;
            return true;
        }
    }
    return false;
}

int PortAudioFrontend::_internal_process_callback(const void* input,
                                                  void* output,
                                                  unsigned long frame_count,
                                                  const PaStreamCallbackTimeInfo* time_info,
                                                  [[maybe_unused]]PaStreamCallbackFlags status_flags)
{
    // TODO: Print warning in case of under/overflow using the status_flags when we have rt-safe logging
    SUSHI_LOG_WARNING_IF(status_flags == paOutputUnderflow, "Detected output underflow in portaudio");
    SUSHI_LOG_WARNING_IF(status_flags == paOutputOverflow, "Detected output overflow in portaudio");
    SUSHI_LOG_WARNING_IF(status_flags == paInputUnderflow, "Detected input underflow in portaudio");
    SUSHI_LOG_WARNING_IF(status_flags == paInputOverflow, "Detected input overflow in portaudio");

    assert(frame_count == AUDIO_CHUNK_SIZE);
    auto pa_time_elapsed = std::chrono::duration<double>(time_info->currentTime - _time_offset);
    Time timestamp = _start_time + std::chrono::duration_cast<std::chrono::microseconds>(pa_time_elapsed);

    _out_buffer.clear();
    if (_pause_manager.should_process())
    {
        _copy_interleaved_audio(static_cast<const float*>(input));
        _engine->process_chunk(&_in_buffer, &_out_buffer, &_in_controls, &_out_controls, timestamp, _processed_sample_count);
        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(_out_buffer);
        }
    }
    else
    {
        if (_pause_notified == false)
        {
            _pause_notify->notify();
            _pause_notified = true;
            _engine->enable_realtime(false);
        }
    }

    _output_interleaved_audio(static_cast<float*>(output));

    _processed_sample_count += frame_count;
    return 0;
}

void PortAudioFrontend::_copy_interleaved_audio(const float* input)
{
    for (int c = 0; c < _num_total_input_channels; c++)
    {
        if (c < _audio_input_channels)
        {
            float* in_dst = _in_buffer.channel(c);
            for (size_t s = 0; s < AUDIO_CHUNK_SIZE; s++)
            {
                in_dst[s] = input[s * _num_total_input_channels + c];
            }
        }
        else
        {
            int cc = c - _audio_input_channels;
            _in_controls.cv_values[cc] = map_audio_to_cv(input[AUDIO_CHUNK_SIZE - 1]);
        }
    }
}

void PortAudioFrontend::_output_interleaved_audio(float* output)
{
    for (int c = 0; c < _num_total_output_channels; c++)
    {
        if (c < _audio_output_channels)
        {
            const float* out_src = _out_buffer.channel(c);
            for (size_t s = 0; s < AUDIO_CHUNK_SIZE; s++)
            {
                output[s * _num_total_output_channels + c] = out_src[s];
            }
        }
        else
        {
            int cc = c - _audio_output_channels;
            _cv_output_his[cc] = ramp_cv_output(output, _cv_output_his[cc], map_cv_to_audio(_out_controls.cv_values[cc]));
        }
    }
}

} // end namespace audio_frontend
} // end namespace sushi
#endif
#ifndef SUSHI_BUILD_WITH_PORTAUDIO
#include "audio_frontends/portaudio_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
SUSHI_GET_LOGGER;
PortAudioFrontend::PortAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{}
AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration*)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    SUSHI_LOG_ERROR("Sushi was not built with PortAudio support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}}}
#endif
