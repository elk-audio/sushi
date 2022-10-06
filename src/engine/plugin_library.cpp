/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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

#include "plugin_library.h"
#include "logging.h"

#include <filesystem>

namespace sushi {
namespace engine {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("plugin_library");

void PluginLibrary::set_base_plugin_path(const std::string& path)
{
    _base_plugin_path = path;
    SUSHI_LOG_WARNING_IF(_base_plugin_path != path,
                         "Overriding previously defined base plugin path: {} with: {}",
                         _base_plugin_path, path);
    SUSHI_LOG_INFO("Setting base plugin path to: {}", _base_plugin_path);
}

std::string PluginLibrary::to_absolute_path(const std::string& path)
{
    auto fspath = std::filesystem::path(path);
    if (fspath.is_absolute() || path.empty())
    {
        return path;
    }

    auto full_path = std::filesystem::path(_base_plugin_path) / fspath;
    return std::string(std::filesystem::absolute(full_path));
}

} // end namespace engine
} // end namespace sushi
