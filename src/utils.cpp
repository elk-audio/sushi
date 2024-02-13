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
 * @brief Utility functions around rapidjson library
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <optional>
#include <iostream>
#include <fstream>

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_TYPE_LIMITS
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
ELK_POP_WARNING

#include "elklog/static_logger.h"
#include "sushi/sushi.h"

namespace sushi {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("utils");

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
        ELKLOG_LOG_ERROR("Invalid path passed to file {}", path);
        return std::nullopt;
    }

    // Iterate through every char in file and store in the string
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());

    return config_file_contents;
}

void init_logger([[maybe_unused]] const SushiOptions& options)
{
    auto ret_code = elklog::StaticLogger::init_logger(options.log_file,
                                                      "Logger",
                                                      options.log_level,
                                                      options.enable_flush_interval? options.log_flush_interval : std::chrono::seconds(0));

    if (ret_code != elklog::Status::OK)
    {
        std::cerr << "Log failure " << ret_code << ", using default." << std::endl;
    }

    if (options.enable_flush_interval)
    {
        ELKLOG_LOG_INFO("Logger flush interval enabled, at {} seconds.",
                        options.log_flush_interval.count());
    }
    else
    {
        ELKLOG_LOG_INFO("Logger flush interval disabled.");
    }
}

}