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
 * @brief Logging wrapper
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <array>
#include <map>
#include <algorithm>

#include "logging.h"
#ifndef SUSHI_DISABLE_LOGGING
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

namespace elk {

std::string Logger::_logger_file_name = "/tmp/sushi.log";
std::string Logger::_logger_name = "Sushi";
spdlog::level::level_enum Logger::_min_log_level = spdlog::level::warn;
std::shared_ptr<spdlog::logger> Logger::logger_instance{nullptr};

SUSHI_LOG_ERROR_CODE Logger::init_logger(const std::string& file_name,
                                         const std::string& logger_name,
                                         const std::string& min_log_level,
                                         const bool enable_flush_interval,
                                         const std::chrono::seconds log_flush_interval)
{
    SUSHI_LOG_ERROR_CODE ret = SUSHI_LOG_ERROR_CODE_OK;

    std::map<std::string, spdlog::level::level_enum> level_map;
    level_map["debug"] = spdlog::level::debug;
    level_map["info"] = spdlog::level::info;
    level_map["warning"] = spdlog::level::warn;
    level_map["error"] = spdlog::level::err;
    level_map["critical"] = spdlog::level::critical;
   
    std::string log_level_lowercase = min_log_level;
    std::transform(min_log_level.begin(), min_log_level.end(), log_level_lowercase.begin(), ::tolower);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%l] %v");

    Logger::_logger_file_name.assign(file_name);
    Logger::_logger_name.assign(logger_name);

    logger_instance = setup_logging();

    if (enable_flush_interval)
    {
        if (log_flush_interval.count() > 0)
        {
            spdlog::flush_every(log_flush_interval);
        }
        else
        {
            return SUSHI_LOG_ERROR_CODE_INVALID_FLUSH_INTERVAL;
        }
        
    }

    if (logger_instance == nullptr)
    {
        ret = SUSHI_LOG_FAILED_TO_START_LOGGER;
    }
    if (level_map.count(log_level_lowercase) > 0)
    {
        spdlog::set_level(level_map[log_level_lowercase]);
    }
    else
    {
        ret = SUSHI_LOG_ERROR_CODE_INVALID_LOG_LEVEL;
    }
    return ret;
}

std::string Logger::get_error_message(SUSHI_LOG_ERROR_CODE status)
{
    constexpr std::array error_messages =
    {
        "Ok",
        "Invalid Log Level",
        "Failed to start logger"
    };

    return error_messages.at(status);
}

std::shared_ptr<spdlog::logger> Logger::setup_logging()
{
    const int  MAX_LOG_FILE_SIZE    = 10'000'000;           // In bytes
    const auto MIN_FLUSH_LEVEL      = spdlog::level::err;   // Min level for automatic flush

    spdlog::set_level(_min_log_level);
    auto async_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(_logger_name,
                                                                               _logger_file_name,
                                                                               MAX_LOG_FILE_SIZE,
                                                                               1);

    async_file_logger->flush_on(MIN_FLUSH_LEVEL);
    async_file_logger->warn("#############################");
    async_file_logger->warn("   Started Sushi Logger!");
    async_file_logger->warn("#############################");
    return async_file_logger;
}

} // end namespace elk

#endif // SUSHI_DISABLE_LOGGING
