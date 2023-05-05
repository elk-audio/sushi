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
* @brief Utilities for working with Apple's CoreAudio
* @Copyright 2017-2023 Elk Audio AB, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_UTILS_H
#define SUSHI_APPLE_COREAUDIO_UTILS_H

#include <string>

#include <mach/mach_time.h>
#include <CoreAudio/AudioHardware.h>

/**
 * Helper macro to log OSStatus errors in a consistent and convenient way.
 */
#define CA_LOG_IF_ERROR(command)                                         \
    do                                                                   \
    {                                                                    \
        OSStatus result = command;                                       \
        if (result != kAudioHardwareNoError)                             \
        {                                                                \
            SUSHI_LOG_ERROR("{} returned error : {}", #command, result); \
        }                                                                \
    } while (false)

/**
 * Helper macro to return with a value when the OSStatus indicates an error.
 */
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
 * Implements the comparison operator for AudioObjectPropertyAddress.
 * @param lhs Left hand side
 * @param rhs Right hand side
 * @return True if equal, or false if not.
 */
inline bool operator==(const AudioObjectPropertyAddress& lhs, const AudioObjectPropertyAddress& rhs)
{
    return lhs.mElement == rhs.mElement && lhs.mScope == rhs.mScope && lhs.mSelector && rhs.mSelector;
}

/**
 * Implements inequality operator for AudioObjectPropertyAddress.
 * @param lhs Left hand side.
 * @param rhs Right hand side.
 * @return True if not equal, or false if it is.
 */
inline bool operator!=(const AudioObjectPropertyAddress& lhs, const AudioObjectPropertyAddress& rhs)
{
    return !(rhs == lhs);
}

namespace apple_coreaudio {

/**
 * Converts given CFString to an std::string.
 * @param cf_string_ref The CFString to convert.
 * @return A standard string with the contents of given CFString, encoded as UTF8.
 */
std::string cf_string_to_std_string(const CFStringRef& cf_string_ref);

/**
 * Little struct which holds host time information and provides facilities to convert from host to real time and vice versa.
 */
struct TimeConversions {
public:
    TimeConversions();

    /**
     * Converts host time to nanoseconds.
     * @param host_time_ticks The host time in ticks.
     * @return The host time in nanoseconds.
     */
    [[nodiscard]] uint64_t host_time_to_nanos(uint64_t host_time_ticks) const;

    /**
     * Converts nanoseconds to host time.
     * @param host_time_nanos Time in nanoseconds.
     * @return The host time in ticks.
     */
    [[nodiscard]] uint64_t nanos_to_host_time(uint64_t host_time_nanos) const;

private:
    // Adapted from CAHostTimeBase.h in the Core Audio Utility Classes
    static uint64_t multiply_by_ratio(uint64_t toMultiply, uint64_t numerator, uint64_t denominator);

    uint64_t _numerator = 0, _denominator = 0;
};

} // namespace apple_coreaudio

#endif // SUSHI_APPLE_COREAUDIO_UTILS_H
