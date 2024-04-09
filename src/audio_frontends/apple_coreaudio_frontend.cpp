/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Realtime audio frontend for Apple CoreAudio
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include "elklog/static_logger.h"

#include "apple_coreaudio_frontend.h"
#include "apple_coreaudio/apple_coreaudio_system_object.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

namespace sushi::internal::audio_frontend {

std::optional<std::string> get_coreaudio_output_device_name(std::optional<std::string> coreaudio_output_device_uid)
{
    auto audio_devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

    if (audio_devices.empty())
    {
        ELKLOG_LOG_ERROR("No Apple CoreAudio devices found");
        return std::nullopt;
    }

    std::string uid;
    if (coreaudio_output_device_uid.has_value())
    {
        uid = coreaudio_output_device_uid.value();
    }
    else
    {
        auto default_audio_output_device_id = apple_coreaudio::AudioSystemObject::get_default_device_id(false); // false to get the output device id
        apple_coreaudio::AudioDevice default_audio_output_device(default_audio_output_device_id);
        uid = default_audio_output_device.uid();
    }

    for (auto& device : audio_devices)
    {
        if (device.uid() == uid)
        {
            return device.name();
        }
    }

    if (coreaudio_output_device_uid.has_value())
    {
        ELKLOG_LOG_ERROR("Could not retrieve device name for coreaudio device with uid: {}",
                        coreaudio_output_device_uid.value());
    }
    else
    {
        ELKLOG_LOG_ERROR("Could not retrieve device name for default coreaudio device, uid: {}", uid);
    }

    return std::nullopt;
}

AppleCoreAudioFrontend::AppleCoreAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

AudioFrontendStatus AppleCoreAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    if (config == nullptr)
    {
        ELKLOG_LOG_ERROR("Invalid config given");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto coreaudio_config = static_cast<AppleCoreAudioFrontendConfiguration*>(config); // NOLINT: Clang-Tidy: Do not use static_cast to downcast from a base to a derived class; use dynamic_cast instead

    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    std::string input_device_uid;
    std::string output_device_uid;

    if (coreaudio_config->input_device_uid->empty())
    {
        auto input_id = apple_coreaudio::AudioSystemObject::get_default_device_id(true);
        apple_coreaudio::AudioDevice default_audio_input_device(input_id);
        input_device_uid = default_audio_input_device.uid();
        ELKLOG_LOG_INFO("Input device not specified, using default: {}", input_device_uid);
    }
    else
    {
        input_device_uid = coreaudio_config->input_device_uid.value();
    }
    if (coreaudio_config->output_device_uid->empty())
    {
        auto output_id = apple_coreaudio::AudioSystemObject::get_default_device_id(false);
        apple_coreaudio::AudioDevice default_audio_output_device(output_id);
        output_device_uid = default_audio_output_device.uid();
        ELKLOG_LOG_INFO("Output device not specified, using default: {}", output_device_uid);
    }
    else
    {
        output_device_uid = coreaudio_config->output_device_uid.value();
    }

    auto devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

    if (input_device_uid == output_device_uid)
    {
        // Input device is same as output device. We're going to open a single device.

        AudioObjectID audio_device_id = 0;
        if (auto* audio_device = device_for_uid(devices, output_device_uid))
        {
            audio_device_id = audio_device->get_audio_object_id();
        }

        if (audio_device_id == 0)
        {
            ELKLOG_LOG_ERROR("Failed to open audio device for specified UID");
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }

        _audio_device = std::make_unique<apple_coreaudio::AudioDevice>(audio_device_id);
    }
    else
    {
        // Input device is not the same as the output device. Let's create an aggregate device.

        auto* input_audio_device = device_for_uid(devices, input_device_uid);
        auto* output_audio_device = device_for_uid(devices, output_device_uid);

        if (input_audio_device == nullptr || output_audio_device == nullptr)
        {
            ELKLOG_LOG_ERROR("Device not found");
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }

        auto aggregate_device = apple_coreaudio::AudioDevice::create_aggregate_device(*input_audio_device, *output_audio_device);

        if (!aggregate_device)
        {
            ELKLOG_LOG_ERROR("Failed to create aggregate device");
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }

        _audio_device = std::move(aggregate_device);
    }

    auto channel_conf_result = configure_audio_channels(coreaudio_config);
    if (channel_conf_result != AudioFrontendStatus::OK)
    {
        ELKLOG_LOG_ERROR("Failed to configure audio channels");
        return channel_conf_result;
    }

    double sample_rate = _engine->sample_rate();

    if (!_audio_device->is_valid())
    {
        ELKLOG_LOG_ERROR("Invalid output device");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    if (!_audio_device->set_buffer_frame_size(AUDIO_CHUNK_SIZE))
    {
        ELKLOG_LOG_ERROR("Failed to set buffer size to {} for output device \"{}\"", AUDIO_CHUNK_SIZE, _audio_device->name());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    if (!_audio_device->set_nominal_sample_rate(sample_rate))
    {
        ELKLOG_LOG_ERROR("Failed to set sample rate to {} for output device \"{}\"", sample_rate, _audio_device->name());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    _set_engine_sample_rate(static_cast<float>(sample_rate));

    UInt32 input_latency = _audio_device->device_latency(true) + _audio_device->selected_stream_latency(true);
    UInt32 output_latency = _audio_device->device_latency(false) + _audio_device->selected_stream_latency(false);

    auto useconds = std::chrono::microseconds(output_latency * 1'000'000 / static_cast<UInt32>(sample_rate));
    _engine->set_output_latency(useconds);

    ELKLOG_LOG_INFO("Stream started, using input latency {}ms and output latency {}ms",
                   input_latency * 1'000 / static_cast<UInt32>(sample_rate),
                   output_latency * 1'000 / static_cast<UInt32>(sample_rate));

    (void) input_latency; // Ignore variable-not-used warning when compiling without Sushi logging (ie. UnitTests)

    return AudioFrontendStatus::OK;
}

void AppleCoreAudioFrontend::cleanup()
{
    if (_engine != nullptr)
    {
        _engine->enable_realtime(false);
    }

    if (!stop_io())
    {
        ELKLOG_LOG_ERROR("Failed to stop audio device(s)");
    }
}

void AppleCoreAudioFrontend::run()
{
    _engine->enable_realtime(true);

    if (!start_io())
    {
        ELKLOG_LOG_ERROR("Failed to start audio device(s)");
    }
}

AudioFrontendStatus AppleCoreAudioFrontend::configure_audio_channels(const AppleCoreAudioFrontendConfiguration* config)
{
    if (config->cv_inputs > 0 || config->cv_outputs > 0)
    {
        ELKLOG_LOG_ERROR("CV ins and outs not supported and must be set to 0");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _device_num_input_channels = _audio_device->num_channels(true);
    _device_num_output_channels = _audio_device->num_channels(false);

    if (_device_num_input_channels < 0 || _device_num_output_channels < 0)
    {
        ELKLOG_LOG_ERROR("Invalid number of channels ({}/{})", _device_num_input_channels, _device_num_output_channels);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto num_input_channels = std::min(_device_num_input_channels, MAX_FRONTEND_CHANNELS);
    auto num_output_channels = std::min(_device_num_output_channels, MAX_FRONTEND_CHANNELS);

    _in_buffer = ChunkSampleBuffer(num_input_channels);
    _out_buffer = ChunkSampleBuffer(num_output_channels);

    _engine->set_audio_input_channels(num_input_channels);
    _engine->set_audio_output_channels(num_output_channels);

    auto status = _engine->set_cv_input_channels(config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        ELKLOG_LOG_ERROR("Failed to setup CV input channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    status = _engine->set_cv_output_channels(config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        ELKLOG_LOG_ERROR("Failed to setup CV output channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    ELKLOG_LOG_DEBUG("Setting up CoreAudio with {} inputs {} outputs", num_input_channels, num_output_channels);

    if (num_input_channels > 0)
    {
        ELKLOG_LOG_INFO("Connected input channels to \"{}\"", _audio_device->name(apple_coreaudio::AudioDevice::Scope::INPUT));
        ELKLOG_LOG_INFO("Input device has {} available channels", _device_num_input_channels);
    }
    else
    {
        ELKLOG_LOG_INFO("No input channels found, not connecting to input device");
    }

    if (num_output_channels > 0)
    {
        ELKLOG_LOG_INFO("Connected output channels to \"{}\"", _audio_device->name(apple_coreaudio::AudioDevice::Scope::OUTPUT));
        ELKLOG_LOG_INFO("Output device has {} available channels", _device_num_output_channels);
    }
    else
    {
        ELKLOG_LOG_INFO("No output channels found, not connecting to output device");
    }

    return AudioFrontendStatus::OK;
}

bool AppleCoreAudioFrontend::start_io()
{
    if (!_audio_device->start_io(this))
    {
        return false;
    }

    return true;
}

bool AppleCoreAudioFrontend::stop_io()
{
    bool result = true;

    if (_audio_device->is_valid() && !_audio_device->stop_io())
    {
        result = false;
    }

    return result;
}

void AppleCoreAudioFrontend::audio_callback(const float* input_data, int num_input_channels, float* output_data, int num_output_channels, int num_samples, uint64_t host_input_time)
{
    _out_buffer.clear();
    // TODO - Are we sure we always get the exact number of samples we request?
    assert(num_samples == AUDIO_CHUNK_SIZE);
    std::chrono::microseconds current_time(_time_conversions.host_time_to_nanos(host_input_time) / 1000);
    _handle_resume(current_time, num_samples);

    if (_pause_manager.should_process())
    {
        _copy_interleaved_audio_to_input_buffer(input_data, num_input_channels);
        _engine->process_chunk(&_in_buffer, &_out_buffer, &_in_controls, &_out_controls, current_time, _processed_sample_count);

        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(_out_buffer);
        }
    }

    _handle_pause(current_time);
    _copy_output_buffer_to_interleaved_buffer(output_data, num_output_channels);

    _processed_sample_count += num_samples;
}

void AppleCoreAudioFrontend::sample_rate_changed(double new_sample_rate)
{
    ELKLOG_LOG_WARNING("Audio device changed sample rate to: {}", new_sample_rate);

#ifdef EXIT_SUSHI_WHEN_AUDIO_DEVICE_CHANGES_TO_INCOMPATIBLE_SAMPLE_RATE
    // The next piece of code is ugly as **** but prevents a lot of engineering to get to what we need:
    // notifying the user of Elk LIVE Desktop that the sample rate of their device has changed.
    // We do that by exiting the application (from a background thread) with a specific return value which
    // gets interpreted by Elk LIVE Desktop as the reason being the sample rate change.
    //
    // Doing this the proper way would look something like this:
    //   - Install some sort of event loop on the main thread
    //   - Allow other threads to schedule work on this event loop
    //   - Allow other threads to signal the event loop to exit (which results in a clean application exit)
    //
    // One way of creating a simple event loop would be to use a concurrent queue like this:
    //
    // moodycamel::BlockingConcurrentQueue<std::function<void()>> q;
    //
    // std::function<bool()> work; // Return value: true to continue the main event loop or false to exit
    //
    // for (;;)
    // {
    //     if (!q.wait_dequeue_timed(work, std::chrono::milliseconds(500))
    //     {
    //         continue; // Nothing dequeued.
    //     }
    //
    //     if (!work)
    //     {
    //         ELKLOG_LOG_ERROR("Received nullptr function");
    //         break;
    //     }
    //
    //     if (!work()) // A return value of false means exit the event loop.
    //     {
    //         break;
    //     }
    // }

    // Assuming the sample rate doesn't change during audio processing, otherwise we would have a race condition because this method gets called from a background thread.
    // Since the sample rate doesn't change during processing, the next line will always read the correct value which is acceptable in this exceptional case.
    if (std::abs(new_sample_rate - _engine->sample_rate()) > 1.0)
    {
        ELKLOG_LOG_WARNING("Exiting Sushi in response to incompatible external sample rate change (return value: {})", EXIT_RETURN_VALUE_ON_INCOMPATIBLE_SAMPLE_RATE_CHANGE);
        exit(EXIT_RETURN_VALUE_ON_INCOMPATIBLE_SAMPLE_RATE_CHANGE);
    }
#endif
}

void AppleCoreAudioFrontend::_copy_interleaved_audio_to_input_buffer(const float* input, int num_channels)
{
    for (int ch = 0; ch < std::min(num_channels, _in_buffer.channel_count()); ch++)
    {
        float* in_dst = _in_buffer.channel(ch);
        for (size_t s = 0; s < AUDIO_CHUNK_SIZE; s++)
        {
            in_dst[s] = input[s * num_channels + ch];
        }
    }
}

void AppleCoreAudioFrontend::_copy_output_buffer_to_interleaved_buffer(float* output, int num_channels)
{
    for (int ch = 0; ch < std::min(num_channels, _out_buffer.channel_count()); ch++)
    {
        const float* out_src = _out_buffer.channel(ch);
        for (size_t s = 0; s < AUDIO_CHUNK_SIZE; s++)
        {
            output[s * num_channels + ch] = out_src[s];
        }
    }
}

} // namespace sushi::internal::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
#include "elklog/static_logger.h"

#include "apple_coreaudio_frontend.h"

ELKLOG_GET_LOGGER;

namespace sushi::internal::audio_frontend {

sushi::internal::audio_frontend::AudioFrontendStatus sushi::internal::audio_frontend::AppleCoreAudioFrontend::init(
        [[maybe_unused]] sushi::internal::audio_frontend::BaseAudioFrontendConfiguration* config)
{
    // The log print needs to be in a cpp file for initialisation order reasons
    ELKLOG_LOG_ERROR("Sushi was not built with CoreAudio support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}

} // namespace sushi::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO
