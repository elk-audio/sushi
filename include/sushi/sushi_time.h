/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Sushi time types and constants
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_TIME_H
#define SUSHI_TIME_H

#include <chrono>

namespace sushi {

/**
 * @brief Type used for timestamps with micro second granularity
 */
typedef std::chrono::microseconds Time;

/**
 * @brief Convenience shorthand for setting timestamp to 0, i.e. process event without delay.
 */
constexpr Time IMMEDIATE_PROCESS = std::chrono::microseconds(0);

/**
 * @brief Get the current time, only for calling from the non-rt part.
 * @return A Time object containing the current time
 */
inline Time get_current_time()
{
    return std::chrono::duration_cast<Time>(std::chrono::steady_clock::now().time_since_epoch());
}

} // end namespace sushi

#endif //SUSHI_TIME_H
