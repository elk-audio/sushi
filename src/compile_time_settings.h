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
    // Doing this the constexpr way takes quite a bit of copied code from the internet,
    // for a small performance gain. First to convert int to char[], then to concatenate char[].
    // Maybe worth it if we reuse those methods throughout, but not just for this once...
    const std::string sushi_version{ std::to_string(SUSHI__VERSION_MAJ) + "." +
                                     std::to_string(SUSHI__VERSION_MIN) + "." +
                                     std::to_string(SUSHI__VERSION_REV)};

    static constexpr char git_commit_hash[] = SUSHI_GIT_COMMIT_HASH;

    static constexpr char build_timestamp[] = SUSHI_BUILD_TIMESTAMP;

    static constexpr int audio_chunk_size = AUDIO_CHUNK_SIZE;

    static constexpr int sample_rate_default = SUSHI_SAMPLE_RATE_DEFAULT;

    static constexpr char log_level_default[] = SUSHI_LOG_LEVEL_DEFAULT;
    static constexpr char log_filename_default[] = SUSHI_LOG_FILENAME_DEFAULT;
    static constexpr char json_filename_default[] = SUSHI_JSON_FILENAME_DEFAULT;
    static constexpr char jack_client_name_default[] = SUSHI_JACK_CLIENT_NAME_DEFAULT;

    static constexpr int osc_server_port = SUSHI_OSC_SERVER_PORT;
    static constexpr int osc_send_port = SUSHI_OSC_SEND_PORT;

    static constexpr char grpc_listening_port[] = SUSHI_GRPC_LISTENING_PORT;

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