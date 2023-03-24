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
#include <iostream>

#include "sushi/logging.h"

#ifndef SUSHI_DISABLE_LOGGING
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

#ifdef SUSHI_BUILD_WITH_SENTRY
#include "sentry_log_sink.h"
#endif

namespace elk {

std::string Logger::_logger_file_name = "/tmp/sushi.log";
std::string Logger::_logger_name = "Sushi";
spdlog::level::level_enum Logger::_min_log_level = spdlog::level::warn;
std::shared_ptr<spdlog::logger> Logger::logger_instance {nullptr};

SUSHI_LOG_ERROR_CODE Logger::init_logger(const std::string& file_name,
                                         const std::string& logger_name,
                                         const std::string& min_log_level,
                                         const bool enable_flush_interval,
                                         const std::chrono::seconds log_flush_interval,
                                         [[maybe_unused]] const std::string& sentry_crash_handler_path,
                                         [[maybe_unused]] const std::string& sentry_dsn)
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

#ifdef SUSHI_BUILD_WITH_SENTRY
    logger_instance->sinks().push_back(std::make_shared<sentry_sink_mt>(sentry_crash_handler_path, sentry_dsn));
#endif

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
    const int MAX_LOG_FILE_SIZE = 10'000'000; // In bytes
    const auto MIN_FLUSH_LEVEL = spdlog::level::err; // Min level for automatic flush

    spdlog::set_level(_min_log_level);

    auto async_file_logger = spdlog::get(_logger_name);

    if (async_file_logger == nullptr)
    {
         async_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(_logger_name,
                                                                               _logger_file_name,
                                                                               MAX_LOG_FILE_SIZE,
                                                                               1);
    }
    else
    {
// TODO: From what I understand this happens because the logger created persists for the duration that the executable where it
//  was created runs.
//  That was never a problem in Sushi, but in Nikkei it is.
//  For now, this is "just" committed to the the Sushi as library branch. But we'll need to make a better solution before merging.
         std::cout << "spdlog already has a logger with that name!" << std::endl;

         logger_instance.reset();

         async_file_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(_logger_name.append(std::to_string(std::rand())),
                                                                               _logger_file_name,
                                                                               MAX_LOG_FILE_SIZE,
                                                                               1);
    }

    async_file_logger->flush_on(MIN_FLUSH_LEVEL);
    async_file_logger->warn("#############################");
    async_file_logger->warn("   Started Sushi Logger!");
    async_file_logger->warn("#############################");

    return async_file_logger;
}

} // end namespace elk

#endif // SUSHI_DISABLE_LOGGING
