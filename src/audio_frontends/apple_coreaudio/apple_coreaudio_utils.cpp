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

std::string apple_coreaudio::cf_string_to_std_string(const CFStringRef& cf_string_ref)
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

    if (c_string != nullptr)
    {
        return c_string;
    }

    // If the above didn't return anything we have to fall back and use CFStringGetCString.
    CFIndex length = CFStringGetLength(cf_string_ref);
    CFIndex max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);

    std::string output(max_size + 1, 0);// Not sure if max_size includes space for null-termination.
    auto result = CFStringGetCString(cf_string_ref, output.data(), max_size, kCFStringEncodingUTF8);
    if (result == 0)
    {
        return {};
    }

    return output;
}
