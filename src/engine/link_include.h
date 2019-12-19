/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
* @Brief Manages include files for Ableton Link. Serves 2 main purposes:
 *       1. If Sushi is not compiled with Link support, this file includes an empty,
 *       "dummy" implementation of Link, just to satisfy compiler requirements.
 *       2. If Sushi is compiled with Link support, this includes all the Link headers
 *       but injects another Clock implementation based on Twine into it. This is
 *       neccesary as calling clock_get_time() is not safe from a Xenomai thread
 *       context.
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/


#ifndef SUSHI_LINK_INCLUDE_H
#define SUSHI_LINK_INCLUDE_H

#ifdef SUSHI_BUILD_WITH_ABLETON_LINK

#include <chrono>
#include <cmath>
#include <ctime>

#include "twine/twine.h"

#include <ableton/link/Controller.hpp>
#include <ableton/util/Log.hpp>
#include <ableton/platforms/asio/Context.hpp>
#include <ableton/platforms/posix/ScanIpIfAddrs.hpp>

namespace sushi {

class RtSafeClock
{
public:
    [[nodiscard]] std::chrono::microseconds micros() const
    {
        auto time = twine::current_rt_time();
        return std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time));
    }
};

} // sushi

namespace ableton::link::platform {

#ifdef LINK_PLATFORM_LINUX
#define LINK_PLATFORM_LINUX_CACHED LINK_PLATFORM_LINUX
using Clock = ::sushi::RtSafeClock;
using IoContext = platforms::asio::Context<platforms::posix::ScanIpIfAddrs, util::NullLog>;
#endif

} // platform::link::ableton

/* Ugly preprocessor hack to prevent Link from adding its own definition of
 * Clock when including platforms/linux/Clock.hpp from link.hpp */
#undef LINK_PLATFORM_LINUX
#include "ableton/Link.hpp"
#ifdef LINK_PLATFORM_LINUX_CACHED
#define LINK_PLATFORM_LINUX_CACHED LINK_PLATFORM_LINUX
#endif

#else //SUSHI_BUILD_WITH_ABLETON_LINK
#include "link_dummy.h"
#endif //SUSHI_BUILD_WITH_ABLETON_LINK

#endif //SUSHI_LINK_INCLUDE_H
