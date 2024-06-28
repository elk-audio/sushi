/*
 * Copyright 2017-2024 Elk Audio AB
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
 * @brief File access and platform helper functions for VST 3.x plugins.
 * @Copyright 2017-2024 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_LIBRARY_VST3X_FILE_UTILS_H
#define SUSHI_LIBRARY_VST3X_FILE_UTILS_H

#include <filesystem>

namespace sushi::internal::vst3 {

/**
 * @brief Remove illegal characters from a string in order to use it as a folder
 *        or file name.
 * @param name the file/folder name to process
 * @return A string with illegal characters replaced with '_'
 */
std::string make_safe_folder_name(std::string name);

/**
 * @brief Extract the preset name from a file path
 * @param The file path to extract from
 * @return The file name path minus the .vstpreset extension
 */
std::string extract_preset_name(const std::filesystem::path& path);

/**
 * @brief Scan the platform specific locations for presets belonging to this plugin
 * @param plugin_name The name of the plugin
 * @param company The plugin vendor
 * @return A std::vector of paths to preset files
 */
std::vector<std::filesystem::path> scan_for_presets(const std::string& plugin_name, const std::string& company);

} // end namespace sushi::internal::vst3

#endif //SUSHI_LIBRARY_VST3X_FILE_UTILS_H
