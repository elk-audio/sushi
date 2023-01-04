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
#endif

#include <optional>
#include <string>

#include <os/workgroup.h>

#include "options.h"

namespace sushi::apple {

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
