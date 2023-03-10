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

#include "apple_coreaudio_utils.h"

namespace apple_coreaudio {

std::string cf_string_to_std_string(const CFStringRef& cf_string_ref)
{
    if (cf_string_ref == nullptr)
    {
        return {};
    }

    // First try the cheap solution (no allocation). Not guaranteed to return anything.
    // Note: for this particular case there is not really a benefit of using this function,
    // because we're going to allocate a new string anyway, however in this case I prefer
    // to use the 'best practice' here to educate myself properly in the future.
    const auto* c_string = CFStringGetCStringPtr(cf_string_ref, kCFStringEncodingUTF8);
    // The memory pointed to by c_string is owned by cf_string_ref.

    if (c_string != nullptr)
    {
        return c_string;
    }

    // If the above didn't return anything we have to fall back and use CFStringGetCString.
    CFIndex length = CFStringGetLength(cf_string_ref);
    CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1; // Include room for \0 termination

    std::string output(max_size, 0);
    if (!CFStringGetCString(cf_string_ref, output.data(), max_size, kCFStringEncodingUTF8))
    {
        return {};
    }

    // Remove trailing zeroes from output
    output.erase(output.find_last_not_of('\0') + 1, std::string::npos);

    return output;
}

TimeConversions::TimeConversions()
{
    mach_timebase_info_data_t info{};
    mach_timebase_info(&info);
    _numerator = info.numer;
    _denominator = info.denom;
}

uint64_t TimeConversions::host_time_to_nanos(uint64_t host_time_ticks) const
{
    return multiply_by_ratio(host_time_ticks, _numerator, _denominator);
}

uint64_t TimeConversions::nanos_to_host_time(uint64_t host_time_nanos) const
{
    return multiply_by_ratio(host_time_nanos, _denominator, _numerator); // NOLINT
}

uint64_t TimeConversions::multiply_by_ratio(uint64_t toMultiply, uint64_t numerator, uint64_t denominator)
{
#if defined(__SIZEOF_INT128__)
    unsigned __int128 result = toMultiply;
#else
    long double result = toMultiply;
#endif

    if (numerator != denominator)
    {
        result *= numerator;
        result /= denominator;
    }

    return (uint64_t) result;
}

} // namespace apple_coreaudio
