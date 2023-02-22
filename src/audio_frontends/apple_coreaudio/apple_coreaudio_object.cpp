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

#include "apple_coreaudio_object.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

namespace apple_coreaudio {

AudioObject::~AudioObject()
{
    // Remove property listeners
    for (auto& listener_address : _property_listeners)
    {
        CA_LOG_IF_ERROR(AudioObjectRemovePropertyListener(_audio_object_id, &listener_address, &_audio_object_property_listener_proc, this));
    }
}

AudioObjectID AudioObject::get_audio_object_id() const
{
    return _audio_object_id;
}

bool AudioObject::is_valid() const
{
    return _audio_object_id != 0;
}

bool AudioObject::has_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    return AudioObjectHasProperty(audio_object_id, &address);
}

UInt32 AudioObject::get_property_data_size(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    UInt32 data_size = 0;
    CA_RETURN_IF_ERROR(AudioObjectGetPropertyDataSize(audio_object_id, &address, 0, nullptr, &data_size), 0);
    return data_size;
}

bool AudioObject::is_property_settable(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    Boolean is_settable = false;
    CA_RETURN_IF_ERROR(AudioObjectIsPropertySettable(audio_object_id, &address, &is_settable), false);
    return is_settable != 0;
}

UInt32 AudioObject::get_property_data(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, UInt32 data_size, void* data)
{
    UInt32 io_data_size = data_size;
    CA_RETURN_IF_ERROR(AudioObjectGetPropertyData(audio_object_id, &address, 0, nullptr, &io_data_size, data), 0);
    return io_data_size;
}

bool AudioObject::set_property_data(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, UInt32 data_size, const void* data)
{
    CA_RETURN_IF_ERROR(AudioObjectSetPropertyData(audio_object_id, &address, 0, nullptr, data_size, data), false);
    return true;
}

std::string AudioObject::get_cfstring_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    const auto* cf_string_ref = get_property<CFStringRef>(audio_object_id, address);
    if (cf_string_ref == nullptr)
    {
        return {};
    }

    auto string = apple_coreaudio::cf_string_to_std_string(cf_string_ref);

    CFRelease(cf_string_ref);

    return string;
}

std::string AudioObject::get_cfstring_property(const AudioObjectPropertyAddress& address) const
{
    return get_cfstring_property(_audio_object_id, address);
}

bool AudioObject::has_property(const AudioObjectPropertyAddress& address) const
{
    return AudioObjectHasProperty(_audio_object_id, &address);
}

bool AudioObject::is_property_settable(const AudioObjectPropertyAddress& address) const
{
    return is_property_settable(_audio_object_id, address);
}

UInt32 AudioObject::get_property_data_size(const AudioObjectPropertyAddress& address) const
{
    return get_property_data_size(_audio_object_id, address);
}

UInt32 AudioObject::get_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, void* data) const
{
    return get_property_data(_audio_object_id, address, data_size, data);
}

bool AudioObject::set_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, const void* data) const
{
    return set_property_data(_audio_object_id, address, data_size, data);
}

bool AudioObject::add_property_listener(const AudioObjectPropertyAddress& address)
{
    for (const auto& listener_address : _property_listeners)
    {
        if (listener_address == address)
            return true;
    }

    CA_RETURN_IF_ERROR(AudioObjectAddPropertyListener(_audio_object_id, &address, &_audio_object_property_listener_proc, this), false);

    _property_listeners.push_back(address);

    return true;
}

OSStatus AudioObject::_audio_object_property_listener_proc(AudioObjectID audio_object_id, UInt32 num_addresses, const AudioObjectPropertyAddress* address, void* client_data)
{
    if (address == nullptr || client_data == nullptr)
    {
        SUSHI_LOG_ERROR("Invalid object passed to _audio_object_property_listener_proc");
        return kAudioHardwareBadObjectError;
    }

    auto* audio_object = static_cast<AudioObject*>(client_data);

    if (audio_object_id != audio_object->_audio_object_id)
    {
        SUSHI_LOG_ERROR("AudioObjectID mismatch (in _audio_object_property_listener_proc)");

        SUSHI_LOG_ERROR("AudioObjectID mismatch (in _audio_object_property_listener_proc)");

        return kAudioHardwareBadObjectError;
    }

    for (UInt32 i = 0; i < num_addresses; i++)
    {
        audio_object->property_changed(address[i]);
    }

    return kAudioHardwareNoError;
}

template<typename T>
std::vector<T> AudioObject::get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    std::vector<T> data_array;
    AudioObject::get_property_array(audio_object_id, address, data_array);
    return data_array;
}

template<typename T>
bool AudioObject::get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, std::vector<T>& data_array)
{
    data_array.resize(0);

    if (!has_property(audio_object_id, address))
    {
        SUSHI_LOG_ERROR("AudioObject doesn't have requested property");
        return false;
    }

    auto data_size = get_property_data_size(audio_object_id, address);

    if (data_size == 0)
    {
        return true; // No data available.
    }

    if (data_size % sizeof(T) != 0)
    {
        SUSHI_LOG_ERROR("Invalid array property size");
        return false;
    }

    auto num_elements = data_size / sizeof(T);
    data_array.resize(num_elements);

    auto num_bytes = data_array.size() * sizeof(T);

    data_size = get_property_data(audio_object_id, address, static_cast<UInt32>(num_bytes), data_array.data());

    // Resize array based on what we actually got.
    data_array.resize(data_size / sizeof(T));

    return true;
}

// Force implementations for these types to be generated, so we can log from this function (which is not possible in the header file).
template std::vector<UInt32> AudioObject::get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);
template std::vector<AudioValueRange> AudioObject::get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

template<typename T>
bool AudioObject::set_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, const T& value)
{
    if (!has_property(audio_object_id, address))
    {
        SUSHI_LOG_ERROR("AudioObject doesn't have requested property");
        return false;
    }

    if (!is_property_settable(audio_object_id, address))
    {
        SUSHI_LOG_ERROR("Property is not settable");
        return false;
    }

    const auto type_size = sizeof(T); // NOLINT Clang-Tidy: Suspicious usage of 'sizeof(A*)'; pointer to aggregate

    if (get_property_data_size(audio_object_id, address) != type_size)
    {
        SUSHI_LOG_ERROR("AudioObject's property size invalid");
        return false;
    }

    return set_property_data(audio_object_id, address, type_size, &value);
}

// Force implementations for these types to be generated, so we can log from this function (which is not possible in the header file).
template bool AudioObject::set_property(AudioObjectID, const AudioObjectPropertyAddress&, const double&);
template bool AudioObject::set_property(AudioObjectID, const AudioObjectPropertyAddress&, const UInt32&);

template<typename T>
T AudioObject::get_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address)
{
    if (!has_property(audio_object_id, address))
    {
        SUSHI_LOG_ERROR("AudioObject doesn't have requested property");
        return {};
    }

    const auto type_size = sizeof(T); // NOLINT Clang-Tidy: Suspicious usage of 'sizeof(A*)'; pointer to aggregate

    if (get_property_data_size(audio_object_id, address) != type_size)
    {
        SUSHI_LOG_ERROR("AudioObject's property size invalid");
        return {};
    }

    T data{};
    auto data_size = get_property_data(audio_object_id, address, type_size, &data);
    if (data_size != type_size)
    {
        SUSHI_LOG_ERROR("Failed to get data from AudioObject");
        return {};
    }

    return data;
}

// Force implementations for these types to be generated, so we can log from this function (which is not possible in the header file).
template double AudioObject::get_property<double>(AudioObjectID, const AudioObjectPropertyAddress&);
template UInt32 AudioObject::get_property<UInt32>(AudioObjectID, const AudioObjectPropertyAddress&);

} // namespace apple_coreaudio
