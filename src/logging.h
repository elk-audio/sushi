/**
 * @brief Singleton wrapper around spdlog and custom logging macros
 *
 * Currently all logs end up in log.txt in the same folder as the executable
 *
 * If -DDISABLE_LOGGING is passed as a compiler argument, all logging code
 * disappears without a trace. Useful for testing and outside releases.
 *
 * Usage:
 * Call MIND_GET_LOGGER before any code in each file.
 *
 * Write to the logger using the MIND_LOG_XXX macros with cppformat style
 * ie: MIND_LOG_INFO("Setting x to {} and y to {}", x, y);
 *
 * spdlog supports ostream style too, but that doesn't work with
 * -DDISABLE_MACROS unfortunately
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>

/* Prevent name collisions with Xenomai that for some inexplicable reason
 * defines 'debug' as a macro */
#ifdef debug
#undef debug
#endif

#define SPDLOG_DEBUG_ON

/* log macros */
#ifndef MIND_DISABLE_LOGGING
#include "spdlog/spdlog.h"

/* Add file and line numbers to debug prints */
#define SUSHI_ENABLE_DEBUG_FILE_AND_LINE_NUM

/* Use this macro  at the top of a source file to declare a local logger */
#define MIND_GET_LOGGER static auto spdlog_instance = mind::Logger::get_logger()

/*
 * Use these macros to log messages. Use cppformat style, ie:
 * MIND_LOG_INFO("Setting x to {} and y to {}", x, y);
 *
 * spdlog supports ostream style, but that doesn't work with
 * -DDISABLE_MACROS unfortunately
 */
#ifdef SUSHI_ENABLE_DEBUG_FILE_AND_LINE_NUM
#define MIND_LOG_DEBUG(msg, ...) spdlog_instance->debug(msg " - [@{} #{}]", ##__VA_ARGS__, __FILE__ , __LINE__)
#else
#define MIND_LOG_DEBUG(...)    spdlog_instance->debug(__VA_ARGS__)
#endif
#define MIND_LOG_INFO(...)     spdlog_instance->info(__VA_ARGS__)
#define MIND_LOG_WARNING(...)  spdlog_instance->warn(__VA_ARGS__)
#define MIND_LOG_ERROR(...)    spdlog_instance->error(__VA_ARGS__)
#define MIND_LOG_CRITICAL(...) spdlog_instance->crit(__VA_ARGS__)

#ifdef SUSHI_ENABLE_DEBUG_FILE_AND_LINE_NUM
#define MIND_LOG_DEBUG_IF(condition, msg, ...) spdlog_instance->debug_if(condition, msg " - [@{} #{}]", ##__VA_ARGS__, __FILE__ , __LINE__)
#else
#define MIND_LOG_DEBUG_IF(condition, ...)    spdlog_instance->debug_if(condition, __VA_ARGS__)
#endif
#define MIND_LOG_INFO_IF(condition, ...)     spdlog_instance->info_if(condition, __VA_ARGS__)
#define MIND_LOG_WARNING_IF(condition, ...)  spdlog_instance->warn_if(condition, __VA_ARGS__)
#define MIND_LOG_ERROR_IF(condition, ...)    spdlog_instance->error_if(condition, __VA_ARGS__)
#define MIND_LOG_CRITICAL_IF(condition, ...) spdlog_instance->crit_if(condition, __VA_ARGS__)

/** Error codes returned by set_logger_params
 */
enum MIND_LOG_ERROR_CODE
{
    MIND_LOG_ERROR_CODE_OK = 0,
    MIND_LOG_ERROR_CODE_INVALID_LOG_LEVEL = 1,
};

/*
 * Call this _before_ instantiating any object that use MIND_GET_LOGGER
 * to change default log parameters, i.e. inside main()
 *
 * See help of Logger::set_logger_params for more details
 */
#define MIND_LOG_SET_PARAMS(FILE_NAME, LOGGER_NAME, MIN_LOG_LEVEL) \
    mind::Logger::set_logger_params(FILE_NAME, LOGGER_NAME, MIN_LOG_LEVEL)


#define MIND_LOG_GET_ERROR_MESSAGE(retcode) \
    mind::Logger::get_error_message(retcode)



namespace mind {

class Logger
{
public:
    static std::shared_ptr<spdlog::logger> get_logger();

    /**
     * @brief Configure logger parameters. This must be called before instantiating
     *        any object that use MIND_GET_LOGGER, e.g. at beginning of main.
     *
     * @param file_name Name of file used to append logs (without extension)
     * @param logger_name Internal name of the logger
     * @param min_log_level Minimum logging level, one of ('debug', 'info', 'warning', 'error', 'critical')
     *
     * @returns Error code, use MIND_LOG_ERROR_MESSAGES to retrieve the message
     *
     * @todo add other logging options, like e.g. logger type, max file size, etc.
     * @todo Check arguments and e.g. return errors
     */
    static MIND_LOG_ERROR_CODE set_logger_params(const std::string file_name,
                                                 const std::string logger_name,
                                                 const std::string min_log_level);


    /**
     * @brief Get string message from error code returned by set_logger_params
     */
    static std::string get_error_message(MIND_LOG_ERROR_CODE status);

    static std::string logger_file_name()
    {
        return _logger_file_name;
    }

    static std::string logger_name()
    {
        return _logger_name;
    }

    static spdlog::level::level_enum min_log_level()
    {
        return _min_log_level;
    }

private:
    Logger() {}

    static std::string _logger_file_name;
    static std::string _logger_name;
    static spdlog::level::level_enum _min_log_level;
};

std::shared_ptr<spdlog::logger> setup_logging();

} // end namespace mind

#else
/* Define empty macros */
#define MIND_GET_LOGGER
#define MIND_LOG_DEBUG(...)
#define MIND_LOG_INFO(...)
#define MIND_LOG_WARNING(...)
#define MIND_LOG_ERROR(...)
#define MIND_LOG_CRITICAL(...)

#define MIND_LOG_SET_PARAMS(FILE_NAME, LOGGER_NAME, MIN_LOG_LEVEL)

#endif

#endif //LOGGING_H
