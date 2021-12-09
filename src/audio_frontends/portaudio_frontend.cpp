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

SUSHI_GET_LOGGER_WITH_MODULE_NAME("PortAudio");

AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto portaudio_config = static_cast<PortAudioConfiguration*>(config);

    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        SUSHI_LOG_ERROR("Error initializing PortAudio: {}", Pa_GetErrorText(err));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    int device_count = Pa_GetDeviceCount();
    int input_device_id = portaudio_config->input_device_id.value_or(Pa_GetDefaultInputDevice());
    if (input_device_id >= device_count)
    {
        SUSHI_LOG_ERROR("Input device id is out of range");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
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
    else if (output_device_id < 0)
    {
        output_device_id = 0;
    }

    _input_device_info = Pa_GetDeviceInfo(input_device_id);
    _output_device_info = Pa_GetDeviceInfo(output_device_id);
    int input_channels = _input_device_info->maxInputChannels;
    int output_channels = _output_device_info->maxOutputChannels;
    _engine->set_audio_input_channels(input_channels);
    _engine->set_audio_output_channels(output_channels);

    PaStreamParameters input_parameters;
    bzero(&input_parameters, sizeof(input_parameters));
    input_parameters.device = input_device_id;
    input_parameters.channelCount = input_channels;
    input_parameters.sampleFormat = paFloat32;
    input_parameters.suggestedLatency = _input_device_info->defaultLowInputLatency;

    PaStreamParameters output_parameters;
    bzero(&output_parameters, sizeof(output_parameters));
    output_parameters.device = output_device_id;
    output_parameters.channelCount = output_channels;
    output_parameters.sampleFormat = paFloat32;
    output_parameters.suggestedLatency = _output_device_info->defaultLowOutputLatency;

    double samplerate = _engine->sample_rate();
    if (_configure_samplerate(input_parameters, output_parameters, &samplerate) == false)
    {
        SUSHI_LOG_ERROR("Failed to configure samplerate");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    err = Pa_OpenStream(&_stream,
                        input_channels > 0 ? &input_parameters : NULL,
                        output_channels > 0 ? &output_parameters : NULL,
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

    _time_offset = Pa_GetStreamTime(_stream);
    _start_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch());

    if (input_channels > 0)
    {
        SUSHI_LOG_DEBUG("Connected input channels to {}", _input_device_info->name);
    }
    else
    {
        SUSHI_LOG_DEBUG("No input channels found not connecting to input device");
    }

    if (output_channels > 0)
    {
        SUSHI_LOG_DEBUG("Connected output channels to {}", _output_device_info->name);
    }
    else
    {
        SUSHI_LOG_DEBUG("No output channels found not connecting to output device");
    }

    return AudioFrontendStatus::OK;
}

void PortAudioFrontend::cleanup()
{
    _engine->enable_realtime(false);
    PaError result = Pa_IsStreamActive(_stream);
    if(result == 1)
    {
        SUSHI_LOG_INFO("Closing PortAudio stream");
        Pa_StopStream(_stream);
    }
    else if(result != paNoError)
    {
        SUSHI_LOG_WARNING("Error while checking for active stream: {}", Pa_GetErrorText(result));
    }
}

void PortAudioFrontend::run()
{
    _engine->enable_realtime(true);
    Pa_StartStream(_stream);
}

bool PortAudioFrontend::_configure_samplerate(const PaStreamParameters& input_parameters,
                                              const PaStreamParameters& output_parameters,
                                              double* samplerate)
{
    if (Pa_IsFormatSupported(&input_parameters, &output_parameters, *samplerate) == false)
    {
        if (Pa_IsFormatSupported(&input_parameters, &output_parameters, _input_device_info->defaultSampleRate))
        {
            *samplerate = _input_device_info->defaultSampleRate;
        }
        else if (Pa_IsFormatSupported(&input_parameters, &output_parameters, _output_device_info->defaultSampleRate))
        {
            *samplerate = _output_device_info->defaultSampleRate;
        }
        else
        {
            return false;
        }
    }
    return true;
}

int PortAudioFrontend::_internal_process_callback(const void* input,
                                                  void* output,
                                                  unsigned long frameCount,
                                                  const PaStreamCallbackTimeInfo* timeInfo,
                                                  [[maybe_unused]]PaStreamCallbackFlags statusFlags)
{
    // TODO: Print warning in case of under/overflow when we have rt-safe logging
    assert(frameCount == AUDIO_CHUNK_SIZE);
    auto pa_time_elapsed = std::chrono::duration<double>(timeInfo->currentTime - _time_offset);
    Time timestamp = _start_time + std::chrono::duration_cast<std::chrono::microseconds>(pa_time_elapsed);

    ChunkSampleBuffer in_buffer(_input_device_info->maxInputChannels);
    in_buffer.from_interleaved(static_cast<const float*>(input));
    ChunkSampleBuffer out_buffer(_input_device_info->maxOutputChannels);
    out_buffer.from_interleaved(static_cast<float*>(output));
    _engine->process_chunk(&in_buffer, &out_buffer, &_in_controls, &_out_controls, timestamp, frameCount);
    out_buffer.to_interleaved(static_cast<float*>(output));
    return 0;
}

}; // end namespace audio_frontend
}; // end namespace sushi
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
