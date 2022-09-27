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
 * @brief Object passed to processors to allow them access to the engine
 *        to query things like time, tempo and to post non rt events.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */


#include "host_control.h"
#include "logging.h"

#include <filesystem>

namespace sushi {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("host_control");

void HostControl::set_base_plugin_path(const std::string& path)
{
    _base_plugin_path = path;
    SUSHI_LOG_WARNING_IF(_base_plugin_path != path,
                         "Overriding previously defined base plugin path: {} with: {}",
                         _base_plugin_path, path);
    SUSHI_LOG_INFO("Setting base plugin path to: {}", _base_plugin_path);
}

std::string HostControl::convert_plugin_path(const std::string& path)
{
    if (_base_plugin_path.length() == 0)
    {
        return path;
    }

    auto full_path = std::filesystem::path(_base_plugin_path) / std::filesystem::path(path);
    return std::string(std::filesystem::absolute(full_path));
}

} // end namespace sushi

