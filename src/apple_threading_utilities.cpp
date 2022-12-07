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

/**
 * Internal methods for setting to the highest thread priority on Apple silicon.
 */
namespace {

/**
 * @brief Sets the thread to realtime - with explicit periodicity defined for Apple.
 *        This is a prerequisite for it to then join the audio thread workgroup.
 * @param period_ms
 * @return status bool
 */
bool _set_current_thread_to_realtime(double period_ms)
{
    const auto thread = pthread_self();

    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);

    const auto ticks_per_ms = (static_cast<double>(timebase.denom) * 1000000.0) /
                              static_cast<double>(timebase.numer);

    const auto period_ticks = static_cast<uint32_t>(std::min(static_cast<double>(std::numeric_limits<uint32_t>::max()),
                                                             period_ms * ticks_per_ms));

    thread_time_constraint_policy_data_t policy;

    // The nominal amount of time between separate processing arrivals.
    policy.period = period_ticks;

    // The maximum amount of real time that may elapse from the start of a separate processing arrival,
    // to the end of computation for logically correct functioning.
    policy.constraint = policy.period;

    // The nominal amount of computation time needed during a separate processing arrival.
    // The thread may be preempted after the computation time has elapsed.
    // If (computation < constraint/2) it will be forced to constraint/2
    // to avoid unintended preemption and associated timer interrupts.
    policy.computation = std::min(static_cast<uint32_t>(50000), policy.period);

    policy.preemptible = true;

    bool status = thread_policy_set(pthread_mach_thread_np(thread),
                                    THREAD_TIME_CONSTRAINT_POLICY,
                                    reinterpret_cast<thread_policy_t>(&policy),
                                    THREAD_TIME_CONSTRAINT_POLICY_COUNT) == KERN_SUCCESS;

    return status;
}

} // Anonymous namespace

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

#if defined(SUSHI_APPLE_THREADING) && (defined(SUSHI_BUILD_WITH_PORTAUDIO) || defined(SUSHI_BUILD_WITH_APPLE_COREAUDIO))

os_workgroup_t get_device_workgroup(const std::string& device_name)
{
    if (__builtin_available(macOS 11.00, *))
    {
        AudioObjectPropertyAddress property_address;
        property_address.mSelector = kAudioHardwarePropertyDevices;
        property_address.mScope = kAudioObjectPropertyScopeWildcard;
        property_address.mElement = kAudioObjectPropertyElementMain;

        UInt32 size;
        OSStatus status;

        status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property_address, 0, nullptr, &size);

        if (status == noErr)
        {
            auto devices = std::make_unique<AudioDeviceID[]>(size);

            status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, nullptr, &size,
                                                devices.get());

            if (status == noErr)
            {
                // To get the number of audio devices.
                auto device_count = static_cast<int>(size) / static_cast<int>(sizeof(AudioDeviceID));

                for (int i = 0; i < device_count; ++i)
                {
                    char name[512];
                    size = sizeof(name);

                    property_address.mSelector = kAudioDevicePropertyDeviceName;

                    status = AudioObjectGetPropertyData(devices[i], &property_address, 0, nullptr, &size, name);

                    if (status == noErr)
                    {
                        auto name_string = std::string(name, static_cast<int>(strlen(name)));

                        if (name_string == device_name)
                        {
                            property_address.mSelector = kAudioDevicePropertyIOThreadOSWorkgroup;

                            status = AudioObjectGetPropertyDataSize(devices[i], &property_address, 0, nullptr, &size);

                            if (status == noErr)
                            {
                                os_workgroup_t _Nonnull workgroup;

                                status = AudioObjectGetPropertyData(devices[i], &property_address, 0, nullptr, &size,
                                                                    &workgroup);

                                if (status == noErr)
                                {
                                    SUSHI_LOG_INFO("Successfully fetched the audio workgroup");

                                    if (os_workgroup_testcancel(workgroup))
                                    {
                                        SUSHI_LOG_ERROR("The fetched audio workgroup has been cancelled");
                                    }

                                    return workgroup;
                                }
                                else
                                {
                                    SUSHI_LOG_ERROR("Failed fetching the audio workgroup");
                                }
                            }
                            else
                            {
                                SUSHI_LOG_ERROR("Failed fetching the audio workgroup property data size");
                            }
                        }
                    }
                    else
                    {
                        SUSHI_LOG_ERROR("Failed fetching an audio device name");
                    }
                }
            }
            else
            {
                SUSHI_LOG_ERROR("Failed fetching the kAudioObjectSystemObject property data");
            }
        }
        else
        {
            SUSHI_LOG_ERROR("Failed fetching the kAudioObjectSystemObject property data size");
        }

        SUSHI_LOG_ERROR("Getting device workgroup failed for device name: {}", device_name);
    }
    else
    {
        SUSHI_LOG_WARNING("MacOS version 11.0 and up is required to fetch workgroup infoı for: {}", device_name);
    }

    return nullptr;
}

#endif

void leave_workgroup_if_needed(MultithreadingData& worker_data)
{
    if (worker_data.initialized && worker_data.p_workgroup != nullptr)
    {
        os_workgroup_leave(worker_data.p_workgroup, &worker_data.join_token);
    }
}

void initialize_thread(MultithreadingData& worker_data)
{
    double period_ms = std::max(1000.0 * AUDIO_CHUNK_SIZE / worker_data.current_sample_rate,
                                1.0);

    bool set_to_realtime_status = _set_current_thread_to_realtime(period_ms);

    if (set_to_realtime_status)
    {
        SUSHI_LOG_INFO("Setting Apple thread realtime status succeeded.");
    }
    else
    {
        SUSHI_LOG_ERROR("Failed setting thread realtime status.");
    }

    if (worker_data.p_workgroup == nullptr)
    {
#if defined(SUSHI_APPLE_THREADING) && (defined(SUSHI_BUILD_WITH_PORTAUDIO) || defined(SUSHI_BUILD_WITH_APPLE_COREAUDIO))
        SUSHI_LOG_INFO("No Apple real-time workgroup will be joined. "
                       "Sushi running multi-threaded on Apple, will only join workgroups on Portaudio and CoreAudio frontends.");
#endif
    }
    else
    {
        bool workgroup_cancelled = os_workgroup_testcancel(worker_data.p_workgroup);

        if (!workgroup_cancelled)
        {
            int result = os_workgroup_join(worker_data.p_workgroup, &worker_data.join_token);

            switch (result)
            {
                case EINVAL:
                {
                    SUSHI_LOG_ERROR("Attempting to join thread workgroup that is already canceled.");
                    break;
                }
                case EALREADY:
                {
                    SUSHI_LOG_ERROR("Attempting to join thread workgroup which thread is already member of.");
                    break;
                }
                default:
                {
                    SUSHI_LOG_INFO("Thread joining Apple real-time audio workgroup was successful.");
                    break;
                }
            }
        }
        else
        {
            SUSHI_LOG_ERROR("Attempting to join Apple thread workgroup that is already canceled.");
        }
    }
}

} // sushi::apple namespace

#endif // SUSHI_APPLE_THREADING

