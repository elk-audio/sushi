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

 /**
 * @Brief Interface used to handle a library of plugins in the target system.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PLUGIN_LIBRARY_H
#define SUSHI_PLUGIN_LIBRARY_H

#include <string>

namespace sushi {
namespace engine {

class PluginLibrary
{
public:
    PluginLibrary() = default;
    virtual ~PluginLibrary() = default;

    /**
     * @brief Set an absolute path to be the base for plugin paths
     *
     * @param path Absolute path of the base plugin folder
     */
    void set_base_plugin_path(const std::string& path);

    /**
     * @brief Convert a relative plugin path to an absolute path,
     *        if a base plugin path has been set.
     *
     * @param path Relative path to plugin inside the base plugin folder.
     *             It is the caller's responsibility
     *             to ensure that this is a proper relative path (not starting with "/").
     *
     * @return Absolute path of the plugin
     */
    std::string to_absolute_path(const std::string& path);

protected:
    std::string _base_plugin_path;
};

} // end namespace engine
} // end namespace sushi
#endif // SUSHI_PLUGIN_LIBRARY_H
