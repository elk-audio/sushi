#include <map>
#include <algorithm>
#include <iostream>

#include "logging.h"
#ifndef MIND_DISABLE_LOGGING
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/async.h"

namespace mind {

// Static variable need to be initialized here
// so that the linker can find them.
//
// See:
// http://stackoverflow.com/questions/14808864/can-i-initialize-static-float-variable-during-runtime
// answer from Edward A.

int logger_flush_interval = 10; //seconds
std::string Logger::_logger_file_name = "/tmp/sushi.log";
std::string Logger::_logger_name = "Sushi";
spdlog::level::level_enum Logger::_min_log_level = spdlog::level::warn;
std::shared_ptr<spdlog::logger> Logger::logger_instance{nullptr};

MIND_LOG_ERROR_CODE Logger::init_logger(const std::string& file_name,
                                        const std::string& logger_name,
                                        const std::string& min_log_level)
{
    MIND_LOG_ERROR_CODE ret = MIND_LOG_ERROR_CODE_OK;

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

    spdlog::flush_every(std::chrono::seconds(logger_flush_interval));

    if (logger_instance == nullptr)
    {
        ret = MIND_LOG_FAILED_TO_START_LOGGER;
    }
    if (level_map.count(log_level_lowercase) > 0)
    {
        spdlog::set_level(level_map[log_level_lowercase]);
    }
    else
    {
        ret = MIND_LOG_ERROR_CODE_INVALID_LOG_LEVEL;
    }
    return ret;
}

std::string Logger::get_error_message(MIND_LOG_ERROR_CODE status)
{
    static std::string error_messages[] = 
    {
        "Ok",
        "Invalid Log Level",
        "Failed to start logger"
    };

    return error_messages[status];
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
    async_file_logger->warn("   Started Mind Logger!");
    async_file_logger->warn("#############################");
    return async_file_logger;
}

} // end namespace mind

#endif // MIND_DISABLE_LOGGING
