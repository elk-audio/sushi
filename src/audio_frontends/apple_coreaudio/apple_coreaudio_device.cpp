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

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

bool apple_coreaudio::AudioDevice::start_io(apple_coreaudio::AudioDevice::AudioCallback* audio_callback, apple_coreaudio::AudioDevice::Scope for_scope)
{
    if (!is_valid() || _io_proc_id != nullptr || audio_callback == nullptr)
    {
        return false;
    }

    _audio_callback = audio_callback;
    _scope = for_scope;

    CA_RETURN_IF_ERROR(AudioDeviceCreateIOProcID(get_audio_object_id(), audio_device_io_proc, this, &_io_proc_id), false);
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

std::string apple_coreaudio::AudioDevice::get_name() const
{
    if (!is_valid())
    {
        return {};
    }

    return get_cfstring_property({kAudioObjectPropertyName,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain});
}

std::string apple_coreaudio::AudioDevice::get_uid() const
{
    if (!is_valid())
    {
        return {};
    }

    return get_cfstring_property({kAudioDevicePropertyDeviceUID,
                                  kAudioObjectPropertyScopeGlobal,
                                  kAudioObjectPropertyElementMain});
}

int apple_coreaudio::AudioDevice::get_num_channels(bool for_input) const
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

    // Use std::vector as underlying storage so that the allocated memory is under RAII (as opposed to malloc/free).
    std::vector<uint8_t> storage(data_size);

    AudioBufferList* audio_buffer_list;
    audio_buffer_list = reinterpret_cast<AudioBufferList*>(storage.data());

    if (get_property_data(pa, data_size, audio_buffer_list) != data_size)
    {
        SUSHI_LOG_ERROR("Invalid data returned");
        return -1;
    }

    UInt32 channel_count = 0;
    for (UInt32 i = 0; i < audio_buffer_list->mNumberBuffers; i++)
        channel_count += audio_buffer_list->mBuffers[i].mNumberChannels;

    if (channel_count > std::numeric_limits<int>::max())
    {
        SUSHI_LOG_ERROR("Integer overflow");
        return -1;
    }

    return static_cast<int>(channel_count);
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

double apple_coreaudio::AudioDevice::get_nominal_sample_rate() const
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

UInt32 apple_coreaudio::AudioDevice::get_device_latency(bool for_input) const
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

UInt32 apple_coreaudio::AudioDevice::get_stream_latency(UInt32 stream_index, bool for_input) const
{
    if (!is_valid())
    {
        return 0;
    }

    auto stream_ids = get_property_array<UInt32>({kAudioDevicePropertyStreams,
                                                  for_input ? kAudioObjectPropertyScopeInput
                                                            : kAudioObjectPropertyScopeOutput,
                                                  kAudioObjectPropertyElementMain});

    if (stream_index >= stream_ids.size())
    {
        SUSHI_LOG_ERROR("Stream for index {} does not exist", stream_index);
        return 0;
    }

    return AudioObject::get_property<UInt32>(stream_ids[stream_index], {kAudioStreamPropertyLatency,
                                                                        for_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
                                                                        kAudioObjectPropertyElementMain});
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
            _audio_callback->sample_rate_changed(get_nominal_sample_rate());
        }
    }
}

OSStatus apple_coreaudio::AudioDevice::audio_device_io_proc(AudioObjectID audio_object_id, const AudioTimeStamp* now, const AudioBufferList* input_data, const AudioTimeStamp* input_time, AudioBufferList* output_data, const AudioTimeStamp* output_time, void* client_data)
{
    auto* audio_device = reinterpret_cast<AudioDevice*>(client_data);
    if (audio_device == nullptr)
    {
        return 0;
    }

    if (audio_object_id != audio_device->get_audio_object_id())
    {
        return 0;// Wrong audio object id.
    }

    if (audio_device->_audio_callback == nullptr)
    {
        return 0;// No audio callback installed.
    }

    audio_device->_audio_callback->audio_callback(audio_device->_scope, now, input_data, input_time, output_data, output_time);

    return 0;
}

const apple_coreaudio::AudioDevice* apple_coreaudio::get_device_for_uid(const std::vector<AudioDevice>& audio_devices, const std::string& uid)
{
    for (auto& device : audio_devices)
    {
        if (device.get_uid() == uid)
        {
            return &device;
        }
    }
    return nullptr;
}
