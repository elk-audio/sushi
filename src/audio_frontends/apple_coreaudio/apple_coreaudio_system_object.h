/*
* Copyright 2017-2023 Elk Audio AB
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
* @Copyright 2017-2023 Elk Audio AB, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_SYSTEM_OBJECT_H
#define SUSHI_APPLE_COREAUDIO_SYSTEM_OBJECT_H

#include "apple_coreaudio_device.h"

namespace apple_coreaudio {
/**
 * This class represents the Core Audio system object of which only one exists, system wide.
 */
class AudioSystemObject
{
public:
    static std::vector<AudioDevice> get_audio_devices()
    {
        auto device_ids = AudioObject::get_property_array<UInt32>(kAudioObjectSystemObject,
                                                                  {kAudioHardwarePropertyDevices,
                                                                   kAudioObjectPropertyScopeGlobal,
                                                                   kAudioObjectPropertyElementMain});

        std::vector<AudioDevice> audio_devices;
        audio_devices.reserve(device_ids.size());

        for (auto& id : device_ids)
        {
            audio_devices.emplace_back(id);
        }

        return audio_devices;
    }

    static AudioObjectID get_default_device_id(bool for_input)
    {
        AudioObjectPropertyAddress pa{for_input ? kAudioHardwarePropertyDefaultInputDevice
                                                : kAudioHardwarePropertyDefaultOutputDevice,
                                      kAudioObjectPropertyScopeGlobal,
                                      kAudioObjectPropertyElementMain};
        return AudioObject::get_property<AudioObjectID>(kAudioObjectSystemObject, pa);
    };
};

} // namespace apple_coreaudio

#endif // SUSHI_APPLE_COREAUDIO_SYSTEM_OBJECT_H
