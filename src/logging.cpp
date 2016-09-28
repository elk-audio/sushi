#include "logging.h"
#ifndef DISABLE_LOGGING

#include <map>
#include <algorithm>

namespace mind {

// Static variable need to be initialized here
// so that the linker can find them.
//
// See:
// http://stackoverflow.com/questions/14808864/can-i-initialize-static-float-variable-during-runtime
// answer from Edward A.

std::string Logger::_logger_file_name = "log";
std::string Logger::_logger_name = "Logger";
spdlog::level::level_enum Logger::_min_log_level = spdlog::level::warn;


MIND_LOG_ERROR_CODE Logger::set_logger_params(const std::string file_name,
                                               const std::string logger_name,
                                               const std::string min_log_level)
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
    if (level_map.count(log_level_lowercase) > 0)
    {
        Logger::_min_log_level = level_map[log_level_lowercase];
    }
    else
    {
        ret = MIND_LOG_ERROR_CODE_INVALID_LOG_LEVEL;
    }

    Logger::_logger_file_name.assign(file_name);
    Logger::_logger_name.assign(logger_name);

    return ret;
}

std::string Logger::get_error_message(MIND_LOG_ERROR_CODE status)
{
    static std::string error_messages[] = 
    {
        "Ok",
        "Invalid Log Level"
    };

    return error_messages[status];

}

std::shared_ptr<spdlog::logger> Logger::get_logger()
{
    /*
     * Note: A static function variable avoids all initialization
     * order issues associated with a static member variable
     */
    static auto spdlog_instance = setup_logging();
    if (!spdlog_instance)
    {
        std::cerr << "Error, logger is not initialized properly!!! " << std::endl;
    }
    return spdlog_instance;
}

std::shared_ptr<spdlog::logger> setup_logging()
{
    /*
     * Note, configuration parameters are defined here to guarantee
     * that they are defined before calling get_logger()
     */
    const size_t LOGGER_QUEUE_SIZE  = 4096;                  // Should be power of 2
    const int  MAX_LOG_FILE_SIZE    = 10000000;              // In bytes
    const bool LOGGER_FORCE_FLUSH   = true;

    spdlog::set_level(Logger::min_log_level());
    spdlog::set_async_mode(LOGGER_QUEUE_SIZE);
    auto async_file_logger = spdlog::rotating_logger_mt(Logger::logger_name(),
                                                        Logger::logger_file_name(),
                                                        MAX_LOG_FILE_SIZE,
                                                        1,
                                                        LOGGER_FORCE_FLUSH);

    async_file_logger->warn("#############################");
    async_file_logger->warn("   Started Mind Logger!");
    async_file_logger->warn("#############################");
    return async_file_logger;
}

} // end namespace mind

#endif // DISABLE_LOGGING
