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
 * @brief Methods for querying all compile - time configuration (build options, version, githash, etc).
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_COMPILE_TIME_SETTINGS_H
#define SUSHI_COMPILE_TIME_SETTINGS_H

#include <string>
#include <array>
#include "generated/version.h"

// TODO Ilias: Look into whether this can be all made constexpr :)
// Interesting exercise, but probably not worth the effort..
class CompileTimeSettings
{
public:
    CompileTimeSettings() = default;
    ~CompileTimeSettings() = default;

    // TODO Ilias: Populate interface version!
    const std::string interface_version = "1.2.3. POPULATE THIS PLACEHOLDER";

    // Doing this the constexpr way takes quite a bit of copied code from the internet,
    // for a small performance gain. First to convert int to char[], then to concatenate char[].
    // Maybe worth it if we reuse those methods throughout, but not just for this once...
    const std::string sushi_version{ std::to_string(SUSHI__VERSION_MAJ) + "." +
                                     std::to_string(SUSHI__VERSION_MIN) + "." +
                                     std::to_string(SUSHI__VERSION_REV)};

    static constexpr std::array enabled_build_options = {
#ifdef SUSHI_BUILD_WITH_VST2
            "vst2",
#endif
#ifdef SUSHI_BUILD_WITH_VST3
            "vst3",
#endif
#ifdef SUSHI_BUILD_WITH_LV2
            "lv2",
#endif
#ifdef SUSHI_BUILD_WITH_JACK
            "jack",
#endif
#ifdef SUSHI_BUILD_WITH_XENOMAI
            "xenomai",
#endif
#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
            "rpc control",
#endif
#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
            "ableton link",
#endif
    };
};

#endif //SUSHI_COMPILE_TIME_SETTINGS_H