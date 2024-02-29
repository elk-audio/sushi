/**
 * @brief An spdlog sink which wraps the Sentry logging functionality
 * @Copyright 2017-2024 Elk Audio AB, Stockholm
 */

#ifndef ELK_SENTRY_LOG_SINK_H
#define ELK_SENTRY_LOG_SINK_H

#include <iostream>
#include <mutex>

#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include "sentry.h"

namespace elk {

template<typename Mutex>
class SentrySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    SentrySink() : spdlog::sinks::base_sink<Mutex>()
    {
    }

    ~SentrySink()
    {
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        const std::string payload(msg.payload.begin(), msg.payload.end());
        const std::string logger_name(msg.logger_name.begin(), msg.logger_name.end());
        switch (msg.level)
        {
        case spdlog::level::info: _add_breadcrumb(payload, logger_name, "info"); break;
        case spdlog::level::debug: _add_breadcrumb(payload, logger_name, "debug"); break;
        case spdlog::level::warn: _add_breadcrumb(payload, logger_name, "warning"); break;
        case spdlog::level::err:
            sentry_capture_event(sentry_value_new_message_event(
            SENTRY_LEVEL_ERROR, // level
            logger_name.c_str(), // logger name
            payload.c_str() // message
            ));
            break;

        default:
            break;
        }
    }

    void flush_() override
    {
       sentry_flush(1000);
    }

private:
    void _add_breadcrumb(const std::string& message, const std::string& category, const std::string& level)
    {
        sentry_value_t crumb = sentry_value_new_breadcrumb("log", message.c_str());
        sentry_value_set_by_key(crumb, "category", sentry_value_new_string(category.c_str()));
        sentry_value_set_by_key(crumb, "level", sentry_value_new_string(level.c_str()));
        sentry_add_breadcrumb(crumb);
    }
};

using sentry_sink_mt = SentrySink<std::mutex>;
using sentry_sink_st = SentrySink<spdlog::details::null_mutex>;

}

#endif // SUSHI_SENTRY_LOG_SINK_H