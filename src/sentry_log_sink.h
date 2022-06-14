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
 * @brief Option parsing
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SENTRY_LOG_SINK_H
#define SUSHI_SENTRY_LOG_SINK_H

#include "iostream"

#include "spdlog/sinks/base_sink.h"
#include "sentry.h"

namespace elk {

template<typename Mutex>
class SentrySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    SentrySink() : spdlog::sinks::base_sink<Mutex>() { init(); }
    explicit SentrySink(std::unique_ptr<spdlog::formatter> formatter) : spdlog::sinks::base_sink<Mutex>(formatter) { init(); }

    void init()
    {
        std::cout << "using crash handler: " << SUSHI_CRASH_HANDLER_PATH << std::endl;
        std::cout << "started sink with dsn: " << SUSHI_SENTRY_DSN << std::endl;
        sentry_options_t *options = sentry_options_new();
        sentry_options_set_handler_path(options, SUSHI_CRASH_HANDLER_PATH);
        sentry_options_set_dsn(options, SUSHI_SENTRY_DSN);
        sentry_init(options);
    }

    ~SentrySink()
    {
        std::cout << "Destroying sentry sink" << std::endl;
        sentry_close();
    }
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        // spdlog::memory_buf_t formatted;
        // spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        const std::string payload(msg.payload.begin(), msg.payload.end());
        const std::string logger_name(msg.logger_name.begin(), msg.logger_name.end());
        switch (msg.level)
        {
        case spdlog::level::info: _add_breadcrumb(payload, logger_name, "info"); break;
        case spdlog::level::debug: _add_breadcrumb(payload, logger_name, "debug"); break;
        case spdlog::level::warn: _add_breadcrumb(payload, logger_name, "warning"); break;
        case spdlog::level::err:
            sentry_capture_event(sentry_value_new_message_event(
            /*   level */ SENTRY_LEVEL_ERROR,
            /*  logger */ logger_name.c_str(),
            /* message */ payload.c_str()
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

#include "spdlog/details/null_mutex.h"
#include <mutex>
using sentry_sink_mt = SentrySink<std::mutex>;
using sentry_sink_st = SentrySink<spdlog::details::null_mutex>;

}

#endif // SUSHI_SENTRY_LOG_SINK_H