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
 * @brief File and platform utility functions for VST 3.x plugins.
 * @Copyright 2017-2024 Elk Audio AB, Stockholm
 */

#include <cstdlib>

#include "elklog/static_logger.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <libloaderapi.h>
#include <shlobj_core.h>
#undef DELETE
#undef ERROR
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include "vst3x_file_utils.h"

namespace sushi::internal::vst3 {

constexpr char VST_PRESET_SUFFIX[] = ".vstpreset";
constexpr int VST_PRESET_SUFFIX_LENGTH = 10;

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("vst3");

std::string make_safe_folder_name(std::string name)
{
    // See https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Locations+Format/Preset+Locations.html
    constexpr std::string_view INVALID_CHARS = "\\*?/:<>|";
    for (char i : INVALID_CHARS)
    {
        std::replace(name.begin(), name.end(), i, '_');
    }
    return name;
}

bool is_hidden(const std::filesystem::directory_entry& entry)
{
#ifdef _WIN32
    return false;
#endif
    return !entry.path().filename().empty() && entry.path().filename().c_str()[0] == '.';
}

std::filesystem::path get_executable_path()
{
#ifdef _WIN32
    std::array<char, 256> buffer;
    buffer[0] = 0;
    int res = GetModuleFileNameA(nullptr, buffer.data(), buffer.size());
    if (res == 0)
    {
        return std::filesystem::absolute(buffer.data());
    }
#elif defined(__APPLE__)
    std::array<char, 256> buffer{0};
    uint32_t  size = buffer.size();
    int res = _NSGetExecutablePath(buffer.data(), &size);
    if (res == 0)
    {
        return std::filesystem::absolute(buffer.data());
    }
#else
    std::array<char, _POSIX_SYMLINK_MAX + 1> buffer{0};
    auto path_length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (path_length > 0)
    {
        return std::filesystem::absolute(buffer.data());
    }
#endif
    ELKLOG_LOG_WARNING("Failed to get binary directory");
    return {};
}

std::vector<std::filesystem::path> get_preset_locations()
{
    std::vector<std::filesystem::path> locations;
#ifdef _WIN32
    PWSTR path = nullptr;
    HRESULT res = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
    if (res == S_OK)
    {
        locations.emplace_back(std::filesystem::path(path) / "VST3 Presets");
    }
    res = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path);
    if (res == S_OK)
    {
        locations.emplace_back(std::filesystem::path(path) / "VST3 Presets");
    }
    res = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &path);
    if (res == S_OK)
    {
        locations.emplace_back(std::filesystem::path(path) / "VST3 Presets");
    }
    auto exe_path = get_executable_path();
    exe_path.remove_filename();
    locations.emplace_back(exe_path / "VST3 Presets");
#elif defined(__APPLE__)
    char* home_dir = getenv("HOME");
    if (home_dir != nullptr)
    {
        locations.push_back(std::filesystem::path(home_dir) / "Library" / "Audio" / "Presets");
    }
    ELKLOG_LOG_WARNING_IF(home_dir == nullptr, "Failed to get home directory")
    locations.emplace_back("/Library/Audio/Presets/");
    locations.emplace_back("/Network/Library/Audio/Presets/");
    auto exe_path = get_executable_path();
    exe_path.remove_filename();
    locations.emplace_back(std::filesystem::absolute(exe_path /".."/ ".." / "VST3 Presets"));
#else
    char* home_dir = getenv("HOME");
    if (home_dir != nullptr)
    {
        locations.push_back(std::filesystem::path(home_dir) / ".vst3" / "presets");
    }
    ELKLOG_LOG_WARNING_IF(home_dir == nullptr, "Failed to get home directory")

    locations.emplace_back("/usr/share/vst3/presets/");
    locations.emplace_back("/usr/local/share/vst3/presets/");
    auto exe_path = get_executable_path();
    exe_path.remove_filename();
    locations.emplace_back(exe_path / "vst3" / "presets");
#endif
    return locations;
}

std::string extract_preset_name(const std::filesystem::path& path)
{
    auto fname = path.filename().string();
    return fname.substr(0, fname.length() - VST_PRESET_SUFFIX_LENGTH);
}

// Recursively search subdirs for preset files
void add_patches(const std::filesystem::path& path, std::vector<std::filesystem::path>& patches)
{
    ELKLOG_LOG_DEBUG("Looking for presets in: {}", path.c_str());
    std::error_code error_code;
    auto items = std::filesystem::directory_iterator(path, error_code);
    if (!error_code)
    {
        ELKLOG_LOG_WARNING("Failed to open directory {} with error {} ({})", path.c_str(), error_code.value(), error_code.message());
        return;
    }
    for (const auto& entry : items)
    {
        if (entry.is_regular_file())
        {
            if (entry.path().string().ends_with(VST_PRESET_SUFFIX))
            {
                ELKLOG_LOG_DEBUG("Reading vst preset patch: {}", entry.path().filename().c_str());
                patches.push_back(entry.path());
            }
        }
        else if (entry.is_directory() && !is_hidden(entry))
        {
            add_patches(entry.path(), patches);
        }
    }
}

std::vector<std::filesystem::path> scan_for_presets(const std::string& plugin_name, const std::string& company)
{
    /* VST3 standard says you should put preset files in specific locations, So we recursively
     * scan these folders for all files that match, just like we do with Re plugins*/
    std::vector<std::filesystem::path> patches;
    std::vector<std::filesystem::path> paths = get_preset_locations();
    for (auto path : paths)
    {
        add_patches(path / company / plugin_name, patches);
    }
    return patches;
}

} // end namespace sushi::internal::vst3
