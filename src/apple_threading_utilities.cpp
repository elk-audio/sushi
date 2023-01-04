/*
 * Copyright 2017-2022 Elk Audio AB
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
 * @brief For Apple silicon, an additional API is exposed besides that for posix threading.
 *        This file collects tools for this Apple threading API.
 * @copyright 2017-2022 Elk Audio AB, Stockholm
 */

#include "apple_threading_utilities.h"

#ifdef SUSHI_APPLE_THREADING

#include <pthread.h>

#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <mach/mach_time.h>
#include <mach/thread_act.h>

#include "logging.h"
#include "library/constants.h"

#ifdef SUSHI_BUILD_WITH_PORTAUDIO
#include "audio_frontends/portaudio_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO
#include "audio_frontends/apple_coreaudio/apple_coreaudio_system_object.h"
#endif

SUSHI_GET_LOGGER_WITH_MODULE_NAME("apple threading");

namespace sushi::apple {

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

std::optional<std::string> get_coreaudio_output_device_name(std::optional<std::string> coreaudio_output_device_uid)
{
    auto audio_devices = apple_coreaudio::AudioSystemObject::get_audio_devices();

    if (coreaudio_output_device_uid.has_value())
    {
        if (audio_devices.empty())
        {
            SUSHI_LOG_ERROR("No Apple CoreAudio devices found");
            return std::nullopt;
        }

        for (auto& device : audio_devices)
        {
           if (device.get_uid() == coreaudio_output_device_uid.value())
           {
                return device.get_name();
           }
        }

        SUSHI_LOG_ERROR("Could not retrieve device name for coreaudio device with uid: {}", coreaudio_output_device_uid.value());
    }
    else
    {
        auto default_audio_device_object_id = apple_coreaudio::AudioSystemObject::get_default_device_id(false);

        for (auto it = audio_devices.begin(); it != audio_devices.end(); it++)
        {
            if (it->get_audio_object_id() == default_audio_device_object_id)
            {
                return it->get_name();
            }
        }

        SUSHI_LOG_ERROR("Could not retrieve device name for default coreaudio device.");
    }

    return std::nullopt;
}

#endif

#ifdef SUSHI_BUILD_WITH_PORTAUDIO

std::optional<std::string> get_portaudio_output_device_name(std::optional<int> portaudio_output_device_id)
{
    int device_index = -1;

    if (portaudio_output_device_id.has_value())
    {
        device_index = portaudio_output_device_id.value();
    }
    else
    {
        device_index = Pa_GetDefaultOutputDevice();
    }

    sushi::audio_frontend::PortAudioFrontend frontend {nullptr};

    auto device_info = frontend.device_info(device_index);

    if (!device_info.has_value())
    {
        SUSHI_LOG_ERROR("Could not retrieve device info for Portaudio device with idx: {}", device_index);
        return std::nullopt;
    }

    return device_info.value().name;
}

#endif // SUSHI_BUILD_WITH_PORTAUDIO

} // sushi::apple namespace

#endif // SUSHI_APPLE_THREADING

