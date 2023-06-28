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
 * @brief Utility functions around rapidjson library
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <optional>
#include <iostream>
#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#pragma GCC diagnostic pop

#include "sushi/logging.h"

namespace sushi {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("file_reading");

std::ostream& operator<<(std::ostream& out, const rapidjson::Document& document)
{
    rapidjson::OStreamWrapper osw(out);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
    document.Accept(writer);
    return out;
}

std::optional<std::string> read_file(const std::string& path)
{
    std::ifstream config_file(path);
    if (!config_file.good())
    {
        SUSHI_LOG_ERROR("Invalid path passed to file {}", path);
        return std::nullopt;
    }

    // Iterate through every char in file and store in the string
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());

    return config_file_contents;
}

}