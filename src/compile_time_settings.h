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
 * @brief Methods for querying compile - time configuration (build options, version, githash, etc).
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_COMPILE_TIME_SETTINGS_H
#define SUSHI_COMPILE_TIME_SETTINGS_H

#include <string>
#include <array>
#include "generated/version.h"
#include "options.h"

struct CompileTimeSettings
{
    static constexpr auto sushi_version = SUSHI_STRINGIZE(SUSHI__VERSION_MAJ) "." SUSHI_STRINGIZE(SUSHI__VERSION_MIN) "." SUSHI_STRINGIZE(SUSHI__VERSION_REV);

    static constexpr auto git_commit_hash = SUSHI_GIT_COMMIT_HASH;

    static constexpr auto build_timestamp = SUSHI_BUILD_TIMESTAMP;

    static constexpr auto audio_chunk_size = AUDIO_CHUNK_SIZE;

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