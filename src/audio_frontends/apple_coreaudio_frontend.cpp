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
#include "logging.h"

#include <CoreAudio/AudioHardware.h>

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

#define CA_LOG_IF_ERROR(command)                                         \
    do                                                                   \
    {                                                                    \
        OSStatus result = command;                                       \
        if (result != kAudioHardwareNoError)                             \
        {                                                                \
            SUSHI_LOG_ERROR("{} returned error : {}", #command, result); \
        }                                                                \
    } while (false)

#define CA_RETURN_IF_ERROR(command, return_value)                        \
    do                                                                   \
    {                                                                    \
        OSStatus result = command;                                       \
        if (result != kAudioHardwareNoError)                             \
        {                                                                \
            SUSHI_LOG_ERROR("{} returned error : {}", #command, result); \
            return return_value;                                         \
        }                                                                \
    } while (false)

/**
 * Converts given CFString to an std::string.
 * @param cf_string_ref The CFString to convert.
 * @return A standard string with the contents of given CFString, encoded as UTF8.
 */
std::string cf_string_to_std_string(const CFStringRef cf_string_ref)
{
    if (cf_string_ref == nullptr)
        return {};

    // First try the cheap solution (no allocation). Not guaranteed to return anything.
    const auto* c_string = CFStringGetCStringPtr(cf_string_ref, kCFStringEncodingUTF8);

    if (c_string != nullptr)
        return c_string;

    // If the above didn't return anything we have to fall back and use CFStringGetCString.
    CFIndex length = CFStringGetLength(cf_string_ref);
    CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);

    std::string output(max_size + 1, 0);// Not sure if max_size includes space for null-termination.
    auto result = CFStringGetCString(cf_string_ref, output.data(), max_size, kCFStringEncodingUTF8);
    if (result == 0)
        return {};

    return output;
}

/**
 * This class represents a numerical audio object as we know from the Core Audio API (AudioHardware.h etc).
 * It also implements basic, common capabilities of an audio object, like getting and setting of properties.
 */
class AudioObject
{
public:
    AudioObject() = delete;

    explicit AudioObject(AudioObjectID audio_object_id) : _audio_object_id(audio_object_id)
    {
    }

    /**
     * @return The AudioObjectID for this AudioObject.
     */
    [[nodiscard]] AudioObjectID get_audio_object_id() const
    {
        return _audio_object_id;
    }

protected:
    /**
     * Gets the data for property of type T.
     * @tparam T The type of the property (it's size must match the size of the property).
     * @param address The address of the property.
     * @return The property, or a default constructed value on error.
     */
    template<typename T>
    [[nodiscard]] T get_property(const AudioObjectPropertyAddress& address) const
    {
        if (!_has_property(address))
        {
            SUSHI_LOG_ERROR("AudioObject doesn't have requested property");
            return {};
        }

        const auto type_size = sizeof(T);// NOLINT Clang-Tidy: Suspicious usage of 'sizeof(A*)'; pointer to aggregate

        if (_get_property_data_size(address) != type_size)
        {
            SUSHI_LOG_ERROR("AudioObject's property size invalid");
            return {};
        }

        T data{};
        auto data_size = _get_property_data(address, type_size, &data);
        if (data_size != type_size)
        {
            SUSHI_LOG_ERROR("Failed to get data from AudioObject");
            return {};
        }

        return data;
    }

    /**
     * Gets an array property.
     * @tparam T The type of the property's elements.
     * @param address The address of the property.
     * @param data_array The array to put the property data into.
     * @return True if successful, or false if an error occurred.
     */
    template<typename T>
    bool get_property_array(const AudioObjectPropertyAddress& address, std::vector<T>& data_array) const
    {
        data_array.resize(0);

        if (!_has_property(address))
        {
            SUSHI_LOG_ERROR("AudioObject doesn't have requested property");
            return false;
        }

        auto data_size = _get_property_data_size(address);

        if (data_size % sizeof(T) != 0)
        {
            SUSHI_LOG_ERROR("Invalid array property size");
            return false;
        }

        auto num_elements = data_size / sizeof(T);
        data_array.resize(num_elements);

        auto num_bytes = data_array.size() * sizeof(T);

        if (num_bytes > std::numeric_limits<UInt32>::max())
        {
            SUSHI_LOG_ERROR("Int overflow");
            return false;
        }

        data_size = _get_property_data(address, static_cast<UInt32>(num_bytes), data_array.data());

        // Resize array based on what we actually got.
        data_array.resize(data_size / sizeof(T));

        return true;
    }

    /**
     * Gets an array property.
     * @tparam T The type of the property's elements.
     * @param address The address of the property.
     * @return An array with the property's data, or an empty array if an error occurred.
     */
    template<typename T>
    [[nodiscard]] std::vector<T> get_property_array(const AudioObjectPropertyAddress& address) const
    {
        std::vector<T> data_array;
        get_property_array(address, data_array);
        return data_array;
    }

private:
    const AudioObjectID _audio_object_id{0};

    /**
     * Tests if this AudioObject has property for given address.
     * @param address The address of the property to lookup.
     * @return True if this object has the property, or false if not.
     */
    [[nodiscard]] bool _has_property(const AudioObjectPropertyAddress& address) const
    {
        return AudioObjectHasProperty(_audio_object_id, &address);
    }

    /**
     * Tests whether the property for given address is settable.
     * @param address The address of the property to lookup.
     * @return True if settable, or false if read-only or non existent.
     */
    [[nodiscard]] bool _is_property_settable(const AudioObjectPropertyAddress& address) const
    {
        Boolean is_settable = false;
        CA_RETURN_IF_ERROR(AudioObjectIsPropertySettable(_audio_object_id, &address, &is_settable), false);
        return is_settable != 0;
    }

    /**
     * Retrieves the data size of the property for given address.
     * @param address The address of the property to lookup.
     * @return The data size of the property, or 0 if the property does not exist or on any other error.
     */
    [[nodiscard]] UInt32 _get_property_data_size(const AudioObjectPropertyAddress& address) const
    {
        UInt32 data_size = 0;
        CA_RETURN_IF_ERROR(AudioObjectGetPropertyDataSize(_audio_object_id, &address, 0, nullptr, &data_size), 0);
        return data_size;
    }

    /**
     * Gets the property data for given address.
     * @param address The address of the property to get the data from.
     * @param data_size The data size of data.
     * @param data The memory of size data_size.
     * @return The actual retrieved size of the data. It might be a lower number than the passed in data_size.
     */
    UInt32 _get_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, void* data) const
    {
        UInt32 io_data_size = data_size;
        CA_RETURN_IF_ERROR(AudioObjectGetPropertyData(_audio_object_id, &address, 0, nullptr, &data_size, data), 0);
        return io_data_size;
    }

    /**
     * Sets the data of property for given address.
     * @param address The address of the property to set the data for.
     * @param data_size The size of the data to set.
     * @param data The data to set.
     * @return True if successful, or false if an error occurred.
     */
    bool _set_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, const void* data) const
    {
        CA_RETURN_IF_ERROR(AudioObjectSetPropertyData(_audio_object_id, &address, 0, nullptr, data_size, data), false);
        return true;
    }
};

/**
 * This class represents a CoreAudio device as a thin C++ wrapper around AudioHardware.h APIs.
 */
class AudioDevice : public AudioObject
{
public:
    explicit AudioDevice(AudioObjectID audio_object_id) : AudioObject(audio_object_id) {}
    virtual ~AudioDevice()
    {
        stop_io();
    }

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    AudioDevice(AudioDevice&& other) noexcept : AudioObject(0)
    {
        *this = std::move(other);// Call into move assignment operator.
    }

    AudioDevice& operator=(AudioDevice&& other) noexcept
    {
        _io_proc_id = other._io_proc_id;
        other._io_proc_id = nullptr;// Prevent other from destroying the proc ID.
        return *this;
    }

    /**
     * Starts IO on this device.
     * @return True if successful, or false if an error occurred.
     */
    bool start_io()
    {
        if (_io_proc_id != nullptr)
            return false;// Already started.

        CA_RETURN_IF_ERROR(AudioDeviceCreateIOProcID(get_audio_object_id(), audio_device_io_proc, this, &_io_proc_id), false);
        CA_RETURN_IF_ERROR(AudioDeviceStart(get_audio_object_id(), _io_proc_id), false);
        return true;
    }

    /**
     * Stops IO on this device.
     * @return True if successful, or false if an error occurred.
     */
    bool stop_io()
    {
        if (_io_proc_id == nullptr)
            return false;

        CA_LOG_IF_ERROR(AudioDeviceStop(get_audio_object_id(), _io_proc_id));
        CA_LOG_IF_ERROR(AudioDeviceDestroyIOProcID(get_audio_object_id(), _io_proc_id));

        _io_proc_id = nullptr;

        return true;
    }

    [[nodiscard]] std::string get_name() const
    {
        const auto* cf_string_ref = get_property<CFStringRef>({kAudioObjectPropertyName,
                                                               kAudioObjectPropertyScopeGlobal,
                                                               kAudioObjectPropertyElementMain});
        if (cf_string_ref == nullptr)
            return {};

        auto string = cf_string_to_std_string(cf_string_ref);

        CFRelease(cf_string_ref);

        return string;
    }

    [[nodiscard]] int get_num_channels(bool for_input) const
    {
        AudioObjectPropertyAddress pa{kAudioDevicePropertyStreamConfiguration,
                                      for_input ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput,
                                      kAudioObjectPropertyElementMain};

        auto audio_buffer_list = get_property<AudioBufferList>(pa);

        UInt32 channel_count = 0;
        for (UInt32 i = 0; i < audio_buffer_list.mNumberBuffers; i++)
            channel_count += audio_buffer_list.mBuffers[i].mNumberChannels;

        if (channel_count > std::numeric_limits<int>::max())
        {
            SUSHI_LOG_ERROR("Integer overflow");
            return -1;
        }

        return static_cast<int>(channel_count);
    }

private:
    AudioDeviceIOProcID _io_proc_id{nullptr};

    /**
     * This function will be called by the realtime audio IO thread to provide and get a new block of audio data.
     * @param audio_object_id
     * @param now
     * @param input_data
     * @param input_time
     * @param output_data
     * @param output_time
     */
    void audio_device_io_proc([[maybe_unused]] const AudioTimeStamp* now,
                              [[maybe_unused]] const AudioBufferList* input_data,
                              [[maybe_unused]] const AudioTimeStamp* input_time,
                              [[maybe_unused]] AudioBufferList* output_data,
                              [[maybe_unused]] const AudioTimeStamp* output_time) {}

    /**
     * Static function which gets called by an audio device to provide and get audio data.
     * @return The return value is currently unused and should always be 0 (see AudioDeviceIOProc in AudioHardware.h).
     */
    static OSStatus audio_device_io_proc(AudioObjectID audio_object_id,
                                         const AudioTimeStamp* now,
                                         const AudioBufferList* input_data,
                                         const AudioTimeStamp* input_time,
                                         AudioBufferList* output_data,
                                         const AudioTimeStamp* output_time,
                                         void* client_data)
    {
        auto* audio_device = reinterpret_cast<AudioDevice*>(client_data);
        if (audio_device == nullptr)
            return 0;

        if (audio_object_id != audio_device->get_audio_object_id())
            return 0;// Wrong audio object id.

        audio_device->audio_device_io_proc(now, input_data, input_time, output_data, output_time);

        return 0;
    }
};

/**
 * This class represents the Core Audio system object of which only one exists, system wide.
 * Implemented as singleton to prevent multiple, unnecessary instances of it.
 */
class AudioSystemObject : private AudioObject
{
public:
    AudioSystemObject(const AudioSystemObject&) = delete;
    AudioSystemObject& operator=(const AudioSystemObject&) = delete;

    static std::vector<AudioDevice> get_audio_devices()
    {
        auto device_ids = get_global_instance().get_property_array<UInt32>({kAudioHardwarePropertyDevices,
                                                                            kAudioObjectPropertyScopeGlobal,
                                                                            kAudioObjectPropertyElementMain});

        std::vector<AudioDevice> audio_devices;
        audio_devices.reserve(device_ids.size());

        for (auto& id : device_ids)
            audio_devices.emplace_back(id);

        return audio_devices;
    }

    static AudioObjectID get_default_device_id(bool for_input)
    {
        return get_global_instance().get_property<AudioObjectID>({for_input ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice,
                                                                  kAudioObjectPropertyScopeGlobal,
                                                                  kAudioObjectPropertyElementMain});
    };

private:
    AudioSystemObject() : AudioObject(kAudioObjectSystemObject) {}

    static AudioSystemObject& get_global_instance()
    {
        static AudioSystemObject instance;
        return instance;
    }
};

namespace sushi::audio_frontend {

AppleCoreAudioFrontend::AppleCoreAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{
}

AudioFrontendStatus AppleCoreAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    return BaseAudioFrontend::init(config);
}

void AppleCoreAudioFrontend::cleanup()
{
}

void AppleCoreAudioFrontend::run()
{
}

void AppleCoreAudioFrontend::pause(bool enabled)
{
    BaseAudioFrontend::pause(enabled);
}

rapidjson::Document AppleCoreAudioFrontend::generate_devices_info_document()
{
    AppleCoreAudioFrontend frontend{nullptr};

    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    auto audio_devices = AudioSystemObject::get_audio_devices();
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
        device_obj.AddMember(rapidjson::Value("inputs", allocator).Move(),
                             rapidjson::Value(device.get_num_channels(true)).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("outputs", allocator).Move(),
                             rapidjson::Value(device.get_num_channels(false)).Move(), allocator);
        devices.PushBack(device_obj.Move(), allocator);
    }
    ca_devices.AddMember(rapidjson::Value("devices", allocator).Move(), devices.Move(), allocator);

    auto add_default_device_index = [&audio_devices, &ca_devices, &allocator](bool for_input) {
        auto default_audio_device_object_id = AudioSystemObject::get_default_device_id(for_input);

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

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO

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

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO
