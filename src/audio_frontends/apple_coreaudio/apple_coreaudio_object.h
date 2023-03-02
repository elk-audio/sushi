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
* @brief C++ representation of the AudioObject as used in the CoreAudio apis.
* @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_OBJECT_H
#define SUSHI_APPLE_COREAUDIO_OBJECT_H

#include <vector>

#include "library/constants.h"
#include "apple_coreaudio_utils.h"

namespace apple_coreaudio {

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

    virtual ~AudioObject();

    SUSHI_DECLARE_NON_COPYABLE(AudioObject)

    AudioObject(AudioObject&& other) noexcept
    {
        *this = std::move(other); // Call into move assignment operator.
    }

    AudioObject& operator=(AudioObject&& other) noexcept
    {
        std::swap(_property_listeners, other._property_listeners);

        _audio_object_id = other._audio_object_id;
        other._audio_object_id = 0;

        return *this;
    }

    /**
     * @return The AudioObjectID for this AudioObject.
     */
    [[nodiscard]] AudioObjectID get_audio_object_id() const;

    /**
     * @return True if this object represents an actual object, or false if the audio object id is 0.
     */
    [[nodiscard]] bool is_valid() const;

    /**
     * Tests if this AudioObject has property for given address.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property to lookup.
     * @return True if this object has the property, or false if not.
     */
    static bool has_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

    /**
     * Gets the data for property of type T.
     * @tparam T The type of the property (it's size must match the size of the property).
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property.
     * @return The property, or a default constructed value on error.
     */
    template<typename T>
    static T get_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

    /**
     * Sets the data for property of type T.
     * @tparam T The type of the property (it's size must match the size of the property).
     * @param address The address of the property.
     * @param audio_object_id The ID of the AudioObject to set the property for.
     * @param value The value to set.
     * @return True if successful, or false if property could not be set.
     */
    template<typename T>
    static bool set_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, const T& value);

    /**
     * Gets an array property.
     * @tparam T The type of the property's elements.
     * @param audio_object_id The ID of the AudioObject to get the array from.
     * @param address The address of the property.
     * @param data_array The array to put the property data into.
     * @return True if successful, or false if an error occurred.
     */
    template<typename T>
    static bool get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, std::vector<T>& data_array);

    /**
     * Gets an array property.
     * @tparam T The type of the property's elements.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property.
     * @return An array with the property's data, or an empty array if an error occurred.
     */
    template<typename T>
    static std::vector<T> get_property_array(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

    /**
     * Retrieves the data size of the property for given address.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property to lookup.
     * @return The data size of the property, or 0 if the property does not exist or on any other error.
     */
    static UInt32 get_property_data_size(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

    /**
     * Tests whether the property for given address is settable.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property to lookup.
     * @return True if settable, or false if read-only or non existent.
     */
    static bool is_property_settable(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

    /**
     * Gets the property data for given address.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property to get the data from.
     * @param data_size The data size of data.
     * @param data The memory of size data_size.
     * @return The actual retrieved size of the data. It might be a lower number than the passed in data_size.
     */
    static UInt32 get_property_data(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, UInt32 data_size, void* _Nonnull data);

    /**
     * Sets the data of property for given address.
     * @param audio_object_id The ID of the AudioObject to set the property on.
     * @param address The address of the property to set the data for.
     * @param data_size The size of the data to set.
     * @param data The data to set.
     * @return True if successful, or false if an error occurred.
     */
    static bool set_property_data(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address, UInt32 data_size, const void* _Nonnull data);

    /**
     * Get a string property for given address.
     * Note: please make sure that the property is of type CFStringRef, otherwise behaviour is undefined.
     * @param audio_object_id The ID of the AudioObject to query.
     * @param address The address of the property.
     * @return A string containing the UTF8 representation of property at given address.
     */
    static std::string get_cfstring_property(AudioObjectID audio_object_id, const AudioObjectPropertyAddress& address);

protected:
    /**
     * Gets the data for property of type T for this AudioObject.
     * @tparam T The type of the property (it's size must match the size of the property).
     * @param address The address of the property.
     * @return The property, or a default constructed value on error.
     */
    template<typename T>
    [[nodiscard]] T get_property(const AudioObjectPropertyAddress& address) const
    {
        return get_property<T>(_audio_object_id, address);
    }

    /**
     * Sets the data for property of type T for this AudioObject.
     * @tparam T The type of the property (it's size must match the size of the property).
     * @param address The address of the property.
     * @param value The value to set.
     * @return True if successful, or false if property could not be set.
     */
    template<typename T>
    [[nodiscard]] bool set_property(const AudioObjectPropertyAddress& address, const T& value) const
    {
        return set_property(_audio_object_id, address, value);
    }

    /**
     * Get a string property for given address for this AudioObject.
     * Note: please make sure that the property is of type CFStringRef, otherwise behaviour is undefined.
     * @param address The address of the property.
     * @return A string containing the UTF8 representation of property at given address.
     */
    [[nodiscard]] std::string get_cfstring_property(const AudioObjectPropertyAddress& address) const;

    /**
     * Gets an array property for this AudioObject.
     * @tparam T The type of the property's elements.
     * @param address The address of the property.
     * @param data_array The array to put the property data into.
     * @return True if successful, or false if an error occurred.
     */
    template<typename T>
    bool get_property_array(const AudioObjectPropertyAddress& address, std::vector<T>& data_array) const
    {
        return get_property_array(_audio_object_id, address, data_array);
    }

    /**
     * Gets an array property for this AudioObject.
     * @tparam T The type of the property's elements.
     * @param address The address of the property.
     * @return An array with the property's data, or an empty array if an error occurred.
     */
    template<typename T>
    [[nodiscard]] std::vector<T> get_property_array(const AudioObjectPropertyAddress& address) const
    {
        return AudioObject::get_property_array<T>(_audio_object_id, address);
    }

    /**
     * Tests if this AudioObject has property for given address for this AudioObject.
     * @param address The address of the property to lookup.
     * @return True if this object has the property, or false if not.
     */
    [[nodiscard]] bool has_property(const AudioObjectPropertyAddress& address) const;

    /**
     * Tests whether the property for given address is settable for this AudioObject.
     * @param address The address of the property to lookup.
     * @return True if settable, or false if read-only or non existent.
     */
    [[nodiscard]] bool is_property_settable(const AudioObjectPropertyAddress& address) const;

    /**
     * Retrieves the data size of the property for this AudioObject at given address.
     * @param address The address of the property to lookup.
     * @return The data size of the property, or 0 if the property does not exist or on any other error.
     */
    [[nodiscard]] UInt32 get_property_data_size(const AudioObjectPropertyAddress& address) const;

    /**
     * Gets the property data for given address.
     * @param address The address of the property to get the data from.
     * @param data_size The data size of data.
     * @param data The memory of size data_size.
     * @return The actual retrieved size of the data. It might be a lower number than the passed in data_size.
     */
    UInt32 get_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, void* _Nonnull data) const;

    /**
     * Sets the data of property for given address for this AudioObject.
     * @param address The address of the property to set the data for.
     * @param data_size The size of the data to set.
     * @param data The data to set.
     * @return True if successful, or false if an error occurred.
     */
    bool set_property_data(const AudioObjectPropertyAddress& address, UInt32 data_size, const void* _Nonnull data) const;

    /**
     * Adds a property listener for given address.
     * @param address The address to install a property listener for.
     * @return True if successful, or false if an error occurred.
     * If a property listener for given address was already installed then the return value will be true.
     */
    bool add_property_listener(const AudioObjectPropertyAddress& address);

    /**
     * Called when a property (for which a listener is installed) changed.
     * @param address The address of the property which changed.
     */
    virtual void property_changed([[maybe_unused]] const AudioObjectPropertyAddress& address) {}

private:
    static OSStatus _audio_object_property_listener_proc(AudioObjectID audio_object_id,
                                                         UInt32 num_addresses,
                                                         const AudioObjectPropertyAddress* _Nonnull address,
                                                         void* __nullable client_data);

    AudioObjectID _audio_object_id{0};
    std::vector<AudioObjectPropertyAddress> _property_listeners;
};

} // namespace apple_coreaudio

#endif
