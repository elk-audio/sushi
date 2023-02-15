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
* @brief Realtime audio frontend for Apple CoreAudio
* @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include "apple_coreaudio_frontend.h"
#include "apple_coreaudio/apple_coreaudio_system_object.h"
#include "logging.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/NSString.h>
#include <Foundation/NSDictionary.h>

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

namespace sushi::audio_frontend {

std::optional<std::string> get_coreaudio_output_device_name(std::optional<std::string> coreaudio_output_device_uid)
{
    auto audio_devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

    if (audio_devices.empty())
    {
        SUSHI_LOG_ERROR("No Apple CoreAudio devices found");
        return std::nullopt;
    }

    std::string id;
    if (coreaudio_output_device_uid.has_value())
    {
        id = coreaudio_output_device_uid.value();
    }
    else
    {
         id = apple_coreaudio::AudioSystemObject::get_default_device_id(false); // false: for output.
    }

    for (auto& device : audio_devices)
    {
        if (device.get_uid() == id)
        {
            return device.get_name();
        }
    }

    if (coreaudio_output_device_uid.has_value())
    {
        SUSHI_LOG_ERROR("Could not retrieve device name for coreaudio device with uid: {}",
                        coreaudio_output_device_uid.value());
    }
    else
    {
        SUSHI_LOG_ERROR("Could not retrieve device name for default coreaudio device, uid: {}", id);
    }

    return std::nullopt;
}

AppleCoreAudioFrontend::AppleCoreAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine), _audio_device(0) {}

AudioFrontendStatus AppleCoreAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    if (config == nullptr)
    {
        SUSHI_LOG_ERROR("Invalid config given");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto coreaudio_config = static_cast<AppleCoreAudioFrontendConfiguration*>(config); // NOLINT: Clang-Tidy: Do not use static_cast to downcast from a base to a derived class; use dynamic_cast instead

    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    if (coreaudio_config->input_device_uid->empty() || coreaudio_config->output_device_uid->empty())
    {
        SUSHI_LOG_ERROR("Invalid device UID");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    if (coreaudio_config->input_device_uid == coreaudio_config->output_device_uid)
    {
        // Input device is same as output device. We're going to open a single device.

        auto devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

        AudioObjectID output_device_id = 0;
        if (auto* output_device = get_device_for_uid(devices, coreaudio_config->output_device_uid.value()))
        {
            output_device_id = output_device->get_audio_object_id();
        }

        if (output_device_id == 0)
        {
            SUSHI_LOG_ERROR("Failed to open audio device for specified UID");
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }

        _audio_device = apple_coreaudio::AudioDevice(output_device_id);
    }
    else
    {
        // Input device is not the same as the output device. Let's create an aggregate device.

        auto aggregate_device_id = _create_aggregate_device(coreaudio_config->input_device_uid.value(), coreaudio_config->output_device_uid.value());

        if (aggregate_device_id == 0)
        {
            SUSHI_LOG_ERROR("Failed to create aggregate device");
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }

        _audio_device = apple_coreaudio::AudioDevice(aggregate_device_id);
    }

    auto channel_conf_result = configure_audio_channels(coreaudio_config);
    if (channel_conf_result != AudioFrontendStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to configure audio channels");
        return channel_conf_result;
    }

    double sample_rate = _engine->sample_rate();

    if (!_audio_device.is_valid())
    {
        SUSHI_LOG_ERROR("Invalid output device");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    if (!_audio_device.set_buffer_frame_size(AUDIO_CHUNK_SIZE))
    {
        SUSHI_LOG_ERROR("Failed to set buffer size to {} for output device \"{}\"", AUDIO_CHUNK_SIZE, _audio_device.get_name());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    if (!_audio_device.set_nominal_sample_rate(sample_rate))
    {
        SUSHI_LOG_ERROR("Failed to set sample rate to {} for output device \"{}\"", sample_rate, _audio_device.get_name());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    UInt32 input_latency;

    input_latency = _audio_device.get_device_latency(true) +
                    _audio_device.get_stream_latency(0, true); // Zero mean primary stream.

    UInt32 output_latency = _audio_device.get_device_latency(false) + _audio_device.get_stream_latency(0, false);

    auto useconds = std::chrono::microseconds(output_latency * 1'000'000 / static_cast<UInt32>(sample_rate));
    _engine->set_output_latency(useconds);

    SUSHI_LOG_INFO("Stream started, using input latency {}ms and output latency {}ms",
                   input_latency * 1'000 / static_cast<UInt32>(sample_rate),
                   output_latency * 1'000 / static_cast<UInt32>(sample_rate));

    (void) input_latency; // Ignore variable-not-used warning when compiling without Sushi logging (ie. UnitTests)

    return AudioFrontendStatus::OK;
}

AudioObjectID AppleCoreAudioFrontend::_create_aggregate_device(const std::string& input_device_uid, const std::string& output_device_uid)
{
    NSString* input_uid = [NSString stringWithUTF8String:input_device_uid.c_str()];
    NSString* output_uid = [NSString stringWithUTF8String:output_device_uid.c_str()];

    NSDictionary* description = @{
        @(kAudioAggregateDeviceUIDKey): @"audio.elk.sushi.aggregate",
        @(kAudioAggregateDeviceIsPrivateKey): @(1),
        @(kAudioAggregateDeviceSubDeviceListKey): @[
            @{
                @(kAudioSubDeviceUIDKey): input_uid,
                @(kAudioSubDeviceDriftCompensationKey): @(0),
                @(kAudioSubDeviceDriftCompensationQualityKey): @(kAudioSubDeviceDriftCompensationMinQuality),
            },
            @{
                @(kAudioSubDeviceUIDKey): output_uid,
                @(kAudioSubDeviceDriftCompensationKey): @(1),
                @(kAudioSubDeviceDriftCompensationQualityKey): @(kAudioSubDeviceDriftCompensationMinQuality),
            },
        ],
    };

    AudioObjectID aggregate_device_id{0};
    OSStatus status = AudioHardwareCreateAggregateDevice((CFDictionaryRef) description, &aggregate_device_id);
    if (status != noErr)
    {
        SUSHI_LOG_ERROR("Failed to create aggregate device");
        return 0;
    }

    return aggregate_device_id;
}

void AppleCoreAudioFrontend::cleanup()
{
    if (_engine != nullptr)
    {
        _engine->enable_realtime(false);
    }

    if (!stop_io())
    {
        SUSHI_LOG_ERROR("Failed to stop audio device(s)");
    }
}

void AppleCoreAudioFrontend::run()
{
    _engine->enable_realtime(true);

    if (!start_io())
    {
        SUSHI_LOG_ERROR("Failed to start audio device(s)");
    }
}

rapidjson::Document AppleCoreAudioFrontend::generate_devices_info_document()
{
    AppleCoreAudioFrontend frontend{nullptr};

    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    auto audio_devices = apple_coreaudio::AudioSystemObject::get_audio_devices();
    if (audio_devices.empty())
    {
        SUSHI_LOG_ERROR("No Apple CoreAudio devices found");
        return document;
    }

    rapidjson::Value ca_devices(rapidjson::kObjectType);
    rapidjson::Value devices(rapidjson::kArrayType);
    for (auto& device : audio_devices)
    {
        rapidjson::Value device_obj(rapidjson::kObjectType);
        device_obj.AddMember(rapidjson::Value("name", allocator).Move(),
                             rapidjson::Value(device.get_name().c_str(), allocator).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("uid", allocator).Move(),
                             rapidjson::Value(device.get_uid().c_str(), allocator).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("inputs", allocator).Move(),
                             rapidjson::Value(device.get_num_channels(true)).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("outputs", allocator).Move(),
                             rapidjson::Value(device.get_num_channels(false)).Move(), allocator);
        devices.PushBack(device_obj.Move(), allocator);
    }
    ca_devices.AddMember(rapidjson::Value("devices", allocator).Move(), devices.Move(), allocator);

    auto add_default_device_index = [&audio_devices, &ca_devices, &allocator](bool for_input) {
        auto default_audio_device_object_id = apple_coreaudio::AudioSystemObject::get_default_device_id(for_input);

        for (auto it = audio_devices.begin(); it != audio_devices.end(); it++)
        {
            if (it->get_audio_object_id() == default_audio_device_object_id)
            {
                ca_devices.AddMember(rapidjson::Value(for_input ? "default_input_device" : "default_output_device", allocator).Move(),
                                     rapidjson::Value(static_cast<uint64_t>(std::distance(audio_devices.begin(), it))).Move(), allocator);
                return;
            }
        }

        SUSHI_LOG_ERROR("Could not retrieve Apple CoreAudio default {} device", for_input ? "input" : "output");
    };

    add_default_device_index(true);
    add_default_device_index(false);

    document.AddMember(rapidjson::Value("apple_coreaudio_devices", allocator), ca_devices.Move(), allocator);

    return document;
}

AudioFrontendStatus AppleCoreAudioFrontend::configure_audio_channels(const AppleCoreAudioFrontendConfiguration* config)
{
    if (config->cv_inputs > 0 || config->cv_outputs > 0)
    {
        SUSHI_LOG_ERROR("CV ins and outs not supported and must be set to 0");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _device_num_input_channels = _audio_device.get_num_channels(true);
    _device_num_output_channels = _audio_device.get_num_channels(false);

    if (_device_num_input_channels < 0 || _device_num_output_channels < 0)
    {
        SUSHI_LOG_ERROR("Invalid number of channels ({}/{})", _device_num_input_channels, _device_num_output_channels);
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
        SUSHI_LOG_ERROR("Failed to setup CV input channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    status = _engine->set_cv_output_channels(config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to setup CV output channels");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    SUSHI_LOG_DEBUG("Setting up CoreAudio with {} inputs {} outputs", num_input_channels, num_output_channels);

    if (num_input_channels > 0)
    {
        SUSHI_LOG_INFO("Connected input channels to {}", _audio_device.get_name());
        SUSHI_LOG_INFO("Input device has {} available channels", _device_num_input_channels);
    }
    else
    {
        SUSHI_LOG_INFO("No input channels found, not connecting to input device");
    }

    if (num_output_channels > 0)
    {
        SUSHI_LOG_INFO("Connected output channels to {}", _audio_device.get_name());
        SUSHI_LOG_INFO("Output device has {} available channels", _device_num_output_channels);
    }
    else
    {
        SUSHI_LOG_INFO("No output channels found, not connecting to output device");
    }

    return AudioFrontendStatus::OK;
}

bool AppleCoreAudioFrontend::start_io()
{
    if (!_audio_device.start_io(this, apple_coreaudio::AudioDevice::Scope::INPUT_OUTPUT))
    {
        return false;
    }

    return true;
}

bool AppleCoreAudioFrontend::stop_io()
{
    bool result = true;

    if (_audio_device.is_valid() && !_audio_device.stop_io())
    {
        result = false;
    }

    return result;
}

void AppleCoreAudioFrontend::audio_callback(apple_coreaudio::AudioDevice::Scope scope,
                                            [[maybe_unused]] const AudioTimeStamp* now,
                                            const AudioBufferList* input_data,
                                            [[maybe_unused]] const AudioTimeStamp* input_time,
                                            AudioBufferList* output_data,
                                            [[maybe_unused]] const AudioTimeStamp* output_time)
{
    _out_buffer.clear();

    // Clear output buffers.
    if (output_data != nullptr)
    {
        for (UInt32 i = 0; i < output_data->mNumberBuffers; i++)
        {
            std::memset(output_data->mBuffers[i].mData, 0, output_data->mBuffers[i].mDataByteSize);
        }
    }

    if (scope != apple_coreaudio::AudioDevice::Scope::INPUT_OUTPUT)
    {
        return; // Callbacks from 2 different devices not implemented
    }

    if (input_data == nullptr || output_data == nullptr)
    {
        return;
    }

    if (input_data->mNumberBuffers <= 0 || output_data->mNumberBuffers <= 0)
    {
        return;
    }

    auto input_frame_count = static_cast<int64_t>(input_data->mBuffers[0].mDataByteSize / input_data->mBuffers[0].mNumberChannels / sizeof(float));
    auto output_frame_count = static_cast<int64_t>(output_data->mBuffers[0].mDataByteSize / output_data->mBuffers[0].mNumberChannels / sizeof(float));

    assert(input_frame_count == AUDIO_CHUNK_SIZE);
    assert(input_frame_count == output_frame_count);

    if (_pause_manager.should_process())
    {
        _copy_interleaved_audio_to_input_buffer(static_cast<const float*>(input_data->mBuffers[0].mData), static_cast<int>(input_data->mBuffers[0].mNumberChannels));

        std::chrono::microseconds host_input_time(_time_conversions.host_time_to_nanos(input_time->mHostTime) / 1000);
        _engine->process_chunk(&_in_buffer, &_out_buffer, &_in_controls, &_out_controls, host_input_time, _processed_sample_count);

        if (_pause_manager.should_ramp())
            _pause_manager.ramp_output(_out_buffer);
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

    _copy_output_buffer_to_interleaved_buffer(static_cast<float*>(output_data->mBuffers[0].mData), static_cast<int>(output_data->mBuffers[0].mNumberChannels));

    _processed_sample_count += input_frame_count;
}

void AppleCoreAudioFrontend::sample_rate_changed(double new_sample_rate)
{
    SUSHI_LOG_WARNING("Audio device changed sample rate to: {}", new_sample_rate);

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
    //         SUSHI_LOG_ERROR("Received nullptr function");
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
        SUSHI_LOG_WARNING("Exiting Sushi in response to incompatible external sample rate change (return value: {})", EXIT_RETURN_VALUE_ON_INCOMPATIBLE_SAMPLE_RATE_CHANGE);
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

} // namespace sushi::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
#include "apple_coreaudio_frontend.h"
#include "logging.h"

SUSHI_GET_LOGGER;

namespace sushi::audio_frontend {

sushi::audio_frontend::AudioFrontendStatus sushi::audio_frontend::AppleCoreAudioFrontend::init(
        [[maybe_unused]] sushi::audio_frontend::BaseAudioFrontendConfiguration* config)
{
    // The log print needs to be in a cpp file for initialisation order reasons
    SUSHI_LOG_ERROR("Sushi was not built with CoreAudio support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}

} // namespace sushi::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO
