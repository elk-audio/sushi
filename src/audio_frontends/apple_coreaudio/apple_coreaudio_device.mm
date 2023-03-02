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

#include "apple_coreaudio_device.h"

#include <Foundation/NSString.h>
#include <Foundation/NSDictionary.h>

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

namespace apple_coreaudio {

/**
 * A class which represents a Core Audio Aggregate Device. This aggregate can have 2 devices, one for input and one for output.
 * The main reason for the existence of this class is to allow passing audio between 2 different devices.
 * Different devices often have different clocks, and a slightly different sample rate, even when the nominal rates are equal.
 * To pass audio between these devices you'd have to add async SRC to account for drift. To avoid doing that manually, this class
 * uses the Core Audio aggregate device API which handles the drift correction transparently.
 */
class AggregateAudioDevice : public AudioDevice
{
public:
    explicit AggregateAudioDevice(AudioObjectID audio_object_id) : AudioDevice(audio_object_id)
    {
        auto sub_devices = get_property_array<UInt32>({kAudioAggregateDevicePropertyActiveSubDeviceList,
                                                       kAudioObjectPropertyScopeGlobal,
                                                       kAudioObjectPropertyElementMain});

        if (!is_aggregate_device())
        {
            SUSHI_LOG_ERROR("AudioDevice is not an aggregate");
            return;
        }

        if (sub_devices.size() < 2)
        {
            SUSHI_LOG_ERROR("Not enough sub devices available");
            return;
        }

        _input_device = AudioDevice(sub_devices[0]);
        _output_device = AudioDevice(sub_devices[1]);

        select_stream(true, 0);                                     // Select the first input stream of the input device.
        select_stream(false, _input_device.num_streams(false)); // Select the first output stream of the output device.
    }

    ~AggregateAudioDevice() override
    {
        stop_io();
        CA_LOG_IF_ERROR(AudioHardwareDestroyAggregateDevice(get_audio_object_id()));
    }

    [[nodiscard]] std::string name(Scope scope) const override
    {
        switch (scope)
        {
            case Scope::INPUT:
                return _input_device.name();
            case Scope::OUTPUT:
                return _output_device.name();
            case Scope::INPUT_OUTPUT:
            case Scope::UNDEFINED:
                return _input_device.name() + " / " + _output_device.name();
        }
    }

    [[nodiscard]] int num_channels(bool for_input) const override
    {
        return for_input ? _input_device.num_channels(true) : _output_device.num_channels(false);
    }

private:
    AudioDevice _input_device;
    AudioDevice _output_device;
};

} // namespace apple_coreaudio

apple_coreaudio::AudioDevice::~AudioDevice()
{
    stop_io();
}

bool apple_coreaudio::AudioDevice::start_io(apple_coreaudio::AudioDevice::AudioCallback* audio_callback)
{
    if (!is_valid() || _io_proc_id != nullptr || audio_callback == nullptr)
    {
        return false;
    }

    _audio_callback = audio_callback;

    CA_RETURN_IF_ERROR(AudioDeviceCreateIOProcID(get_audio_object_id(), _audio_device_io_proc, this, &_io_proc_id), false);
    CA_RETURN_IF_ERROR(AudioDeviceStart(get_audio_object_id(), _io_proc_id), false);

    if (!add_property_listener({kAudioDevicePropertyNominalSampleRate,
                                kAudioObjectPropertyScopeGlobal,
                                kAudioObjectPropertyElementMain}))
    {
        SUSHI_LOG_ERROR("Failed to install property listener for sample rate change");
    }

    return true;
}

bool apple_coreaudio::AudioDevice::stop_io()
{
    if (!is_valid() || _io_proc_id == nullptr)
    {
        return false;
    }

    CA_LOG_IF_ERROR(AudioDeviceStop(get_audio_object_id(), _io_proc_id));
    CA_LOG_IF_ERROR(AudioDeviceDestroyIOProcID(get_audio_object_id(), _io_proc_id));

    _io_proc_id = nullptr;
    _audio_callback = nullptr;

    return true;
}

std::string apple_coreaudio::AudioDevice::name() const
{
    if (!is_valid())
    {
        return {};
    }

    return get_cfstring_property({kAudioObjectPropertyName,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain});
}

std::string apple_coreaudio::AudioDevice::name(apple_coreaudio::AudioDevice::Scope) const
{
    return name();
}

std::string apple_coreaudio::AudioDevice::uid() const
{
    if (!is_valid())
    {
        return {};
    }

    return get_cfstring_property({kAudioDevicePropertyDeviceUID,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain});
}

int apple_coreaudio::AudioDevice::num_channels(bool for_input) const
{
    if (!is_valid())
    {
        return -1;
    }

    AudioObjectPropertyAddress pa{kAudioDevicePropertyStreamConfiguration,
                                  for_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
                                  kAudioObjectPropertyElementMain};

    if (!has_property(pa))
    {
        return -1;
    }

    auto data_size = get_property_data_size(pa);

    if (data_size == 0)
    {
        return -1;
    }

    // Use std::vector as underlying storage so that the allocated memory is under RAII.
    std::vector<uint8_t> storage(data_size);

    AudioBufferList* audio_buffer_list;
    audio_buffer_list = reinterpret_cast<AudioBufferList*>(storage.data());

    if (get_property_data(pa, data_size, audio_buffer_list) != data_size)
    {
        SUSHI_LOG_ERROR("Invalid data returned");
        return -1;
    }

    if (audio_buffer_list->mNumberBuffers == 0)
    {
        return 0;
    }

    auto selected_stream_index = for_input ? _selected_input_stream_index : _selected_output_stream_index;
    if (selected_stream_index >= audio_buffer_list->mNumberBuffers)
    {
        SUSHI_LOG_ERROR("Invalid stream index");
        return -1;
    }

    UInt32 channel_count = audio_buffer_list->mBuffers[selected_stream_index].mNumberChannels;

    if (channel_count > std::numeric_limits<int>::max())
    {
        SUSHI_LOG_ERROR("Integer overflow");
        return -1;
    }

    return static_cast<int>(channel_count);
}

size_t apple_coreaudio::AudioDevice::num_streams(bool for_input) const
{
    return _stream_ids(for_input).size();
}

bool apple_coreaudio::AudioDevice::set_buffer_frame_size(uint32_t buffer_frame_size) const
{
    if (!is_valid())
    {
        return false;
    }

    AudioObjectPropertyAddress pa{kAudioDevicePropertyBufferFrameSize,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain};

    return set_property(pa, buffer_frame_size);
}

bool apple_coreaudio::AudioDevice::set_nominal_sample_rate(double sample_rate) const
{
    if (!is_valid())
    {
        return false;
    }

    AudioObjectPropertyAddress pa{kAudioDevicePropertyNominalSampleRate,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain};

    return set_property(pa, sample_rate);
}

double apple_coreaudio::AudioDevice::nominal_sample_rate() const
{
    if (!is_valid())
    {
        return 0.0;
    }

    AudioObjectPropertyAddress pa{kAudioDevicePropertyNominalSampleRate,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain};

    return get_property<double>(pa);
}

UInt32 apple_coreaudio::AudioDevice::device_latency(bool for_input) const
{
    if (!is_valid())
    {
        return 0;
    }

    AudioObjectPropertyAddress pa{kAudioDevicePropertyLatency,
                                  for_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
                                  kAudioObjectPropertyElementMain};

    return get_property<UInt32>(pa);
}

std::vector<UInt32> apple_coreaudio::AudioDevice::_stream_ids(bool for_input) const
{
    return get_property_array<UInt32>({kAudioDevicePropertyStreams,
                                       for_input ? kAudioObjectPropertyScopeInput
                                                 : kAudioObjectPropertyScopeOutput,
                                       kAudioObjectPropertyElementMain});
}

UInt32 apple_coreaudio::AudioDevice::stream_latency(UInt32 stream_index, bool for_input) const
{
    if (!is_valid())
    {
        return 0;
    }

    auto stream_ids = _stream_ids(for_input);

    if (stream_index >= stream_ids.size())
    {
        SUSHI_LOG_ERROR("Stream for index {} does not exist", stream_index);
        return 0;
    }

    return AudioObject::get_property<UInt32>(stream_ids[stream_index], {kAudioStreamPropertyLatency,
                                                                        for_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
                                                                        kAudioObjectPropertyElementMain});
}

UInt32 apple_coreaudio::AudioDevice::selected_stream_latency(bool for_input) const
{
    return stream_latency(for_input ? _selected_input_stream_index : _selected_output_stream_index, for_input);
}

UInt32 apple_coreaudio::AudioDevice::clock_domain_id() const
{
    if (!is_valid())
    {
        return 0;
    }

    return get_property<UInt32>({kAudioDevicePropertyClockDomain,
                                 kAudioObjectPropertyScopeGlobal,
                                 kAudioObjectPropertyElementMain});
}

std::vector<UInt32> apple_coreaudio::AudioDevice::related_devices() const
{
    auto ids = get_property_array<UInt32>({kAudioDevicePropertyRelatedDevices,
                                           kAudioObjectPropertyScopeGlobal,
                                           kAudioObjectPropertyElementMain});
    return ids;
}

void apple_coreaudio::AudioDevice::property_changed(const AudioObjectPropertyAddress& address)
{
    // Note: this function most likely gets called from a background thread (most likely because there is no official specification on this).

    // Nominal sample rate
    if (address == AudioObjectPropertyAddress{kAudioDevicePropertyNominalSampleRate,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMain})
    {
        if (_audio_callback)
        {
            _audio_callback->sample_rate_changed(nominal_sample_rate());
        }
    }
}

OSStatus apple_coreaudio::AudioDevice::_audio_device_io_proc(AudioObjectID audio_object_id,
                                                             const AudioTimeStamp*,
                                                             const AudioBufferList* input_data,
                                                             const AudioTimeStamp* input_time,
                                                             AudioBufferList* output_data,
                                                             const AudioTimeStamp*,
                                                             void* client_data)
{
    auto* audio_device = reinterpret_cast<AudioDevice*>(client_data);
    if (audio_device == nullptr)
    {
        return 0;
    }

    if (audio_object_id != audio_device->get_audio_object_id())
    {
        return 0; // Wrong audio object id.
    }

    if (audio_device->_audio_callback == nullptr)
    {
        return 0; // No audio callback installed.
    }

    if (output_data == nullptr)
    {
        return 0;
    }

    // Clear output buffers.
    for (UInt32 i = 0; i < output_data->mNumberBuffers; i++)
    {
        std::memset(output_data->mBuffers[i].mData, 0, output_data->mBuffers[i].mDataByteSize);
    }

    // Do this check after the output buffers have been cleared.
    if (input_data == nullptr)
    {
        return 0;
    }

    auto input_stream_index = audio_device->_selected_input_stream_index;
    auto output_stream_index = audio_device->_selected_output_stream_index;

    if (input_data->mNumberBuffers <= input_stream_index || output_data->mNumberBuffers <= output_stream_index)
    {
        return 0;
    }

    auto input_frame_count = static_cast<int32_t>(input_data->mBuffers[input_stream_index].mDataByteSize / input_data->mBuffers[input_stream_index].mNumberChannels / sizeof(float));
    auto output_frame_count = static_cast<int32_t>(output_data->mBuffers[output_stream_index].mDataByteSize / output_data->mBuffers[output_stream_index].mNumberChannels / sizeof(float));

    assert(input_frame_count == AUDIO_CHUNK_SIZE);
    assert(input_frame_count == output_frame_count);

    audio_device->_audio_callback->audio_callback(static_cast<const float*>(input_data->mBuffers[input_stream_index].mData),
                                                  static_cast<int>(input_data->mBuffers[input_stream_index].mNumberChannels),
                                                  static_cast<float*>(output_data->mBuffers[output_stream_index].mData),
                                                  static_cast<int>(output_data->mBuffers[output_stream_index].mNumberChannels),
                                                  std::min(input_frame_count, output_frame_count),
                                                  input_time->mHostTime);

    return 0;
}

std::unique_ptr<apple_coreaudio::AudioDevice> apple_coreaudio::AudioDevice::create_aggregate_device(const AudioDevice& input_device,
                                                                                                    const AudioDevice& output_device)
{
    if (!input_device.is_valid() || !output_device.is_valid())
    {
        return nullptr;
    }

    if (input_device.is_aggregate_device())
    {
        SUSHI_LOG_ERROR("Input device \"{}\" is an aggregate device which cannot be part of another aggregate device",
                        input_device.name());
        return nullptr;
    }

    if (output_device.is_aggregate_device())
    {
        SUSHI_LOG_ERROR("Output device \"{}\" is an aggregate device which cannot be part of another aggregate device",
                        output_device.name());
        return nullptr;
    }

    NSString* input_uid = [NSString stringWithUTF8String:input_device.uid().c_str()];
    NSString* output_uid = [NSString stringWithUTF8String:output_device.uid().c_str()];

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
        return nullptr;
    }

    auto aggregate_device = std::make_unique<AggregateAudioDevice>(aggregate_device_id);

    auto device = apple_coreaudio::AudioDevice(aggregate_device_id);

    return aggregate_device;
}

apple_coreaudio::AudioDevice& apple_coreaudio::AudioDevice::operator=(apple_coreaudio::AudioDevice&& other) noexcept
{
    // Since we're going to adopt another AudioDeviceID we must stop any audio IO proc.
    stop_io();

    // Don't transfer ownership of _io_proc_id because CoreAudio has registered the pointer
    // to other as client data, so let other stop the callbacks when it goes out of scope.
    AudioObject::operator=(std::move(other));
    return *this;
}

bool apple_coreaudio::AudioDevice::is_aggregate_device() const
{
    return get_property<UInt32>({kAudioObjectPropertyClass,
                                 kAudioObjectPropertyScopeGlobal,
                                 kAudioObjectPropertyElementMain}) == kAudioAggregateDeviceClassID;
}

void apple_coreaudio::AudioDevice::select_stream(bool for_input, size_t selected_stream_index)
{
    if (selected_stream_index >= num_streams(for_input))
    {
        return;
    }

    for_input ? _selected_input_stream_index = selected_stream_index : _selected_output_stream_index = selected_stream_index;
}

const apple_coreaudio::AudioDevice* apple_coreaudio::device_for_uid(const std::vector<AudioDevice>& audio_devices,
                                                                        const std::string& uid)
{
    for (auto& device : audio_devices)
    {
        if (device.uid() == uid)
        {
            return &device;
        }
    }
    return nullptr;
}
