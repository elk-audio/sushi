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
* @brief OSC utiliities
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/
#ifndef SUSHI_OSC_UTILS_H
#define SUSHI_OSC_UTILS_H

namespace sushi {
namespace osc {

/**
 * @brief Ensure that a string is safe to use as an OSC path by stripping illegal
 *        characters and replacing spaces with underscores.
 * @param name The string to process
 * @return an std::string safe to use as an OSC path
 */
std::string make_safe_path(std::string name);

} // namespace osc
} // namespace sushi
#endif //SUSHI_OSC_UTILS_H
