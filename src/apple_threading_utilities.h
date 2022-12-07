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

#ifndef SUSHI_APPLE_THREADING_UTILITIES_H
#define SUSHI_APPLE_THREADING_UTILITIES_H

/**
 * This logic switches the use of apple silicon-specific threading priorities and workgroups on/off,
 *  throughout Sushi.
 *  By default it will always be on when building on Apple.
 *  But you may want to temporarily bypass that here, for testing purposes.
 */
#ifdef __APPLE__
#define SUSHI_APPLE_THREADING
#else
typedef void* os_workgroup_t;
#endif

#include <optional>
#include <string>

#include <os/workgroup.h>

#include "options.h"

namespace sushi::apple {

/**
 * A structure defining what data need to be stored for each audio-rate worker thread that is part of the
 * audio workgroup.
 * This data is passed to the thread's callback method as a field member of the WorkerData structure - If on Apple.
 * If not, it is excluded.
 */
struct MultithreadingData
{
    bool initialized = false;

    std::optional<std::string> device_name = std::nullopt;

    os_workgroup_t p_workgroup = nullptr;
    os_workgroup_join_token_s join_token;
    double current_sample_rate = SUSHI_SAMPLE_RATE_DEFAULT;

    // Atomic since it's set in AudioGraph destructor while worker may be running.
    std::atomic<bool> should_leave = false;
};

/**
 * @brief Given an audio output device name, this attempts to fetch and return an Apple audio thread workgroup.
 * @param device_name A validated audio device name.
 * @return The os_workgroup_t found. Apparently this can be nullptr, on failure.
 */
os_workgroup_t get_device_workgroup(const std::string& device_name);

/**
 * @brief This removes the current thread from a workgroup, if it has previously joined it.
 * Threads must leave all workgroups in the reverse order that they have joined them.
 * Failing to do so before exiting will result in undefined behavior.
 * @param worker_data WorkgroupMemberData structure.
 */
void leave_workgroup_if_needed(MultithreadingData& worker_data);

/**
 * @brief Sets the thread to realtime, and joins the audio thread workgroup if possible
 * @param worker_data WorkgroupMemberData structure.
 */
void initialize_thread(MultithreadingData& worker_data);

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

std::optional<std::string> get_coreaudio_output_device_name(std::optional<std::string> coreaudio_output_device_uid);

#endif

#ifdef SUSHI_BUILD_WITH_PORTAUDIO

/**
 * @brief Given an optional portaudio output device ID, this attempts to fetch the corresponding name.
 *        If no id is passed (the optional argument has no value), the default output device is used.
 * @param portaudio_output_device_id an optional int device id.
 * @return An optional std::string, if a value can be resolved.
 */
std::optional<std::string> get_portaudio_output_device_name(std::optional<int> portaudio_output_device_id);

#endif

} // Sushi Apple namespace

#endif // SUSHI_APPLE_THREADING_UTILITIES_H
