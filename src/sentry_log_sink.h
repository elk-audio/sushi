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
 * @brief An spdlog sink which wraps the Sentry logging functionality
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_SENTRY_LOG_SINK_H
#define SUSHI_SENTRY_LOG_SINK_H

#include <iostream>
#include <mutex>

#include "spdlog/details/null_mutex.h"
#include "spdlog/sinks/base_sink.h"
#include "sentry.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sentry");

namespace elk {

template<typename Mutex>
class SentrySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    SentrySink(const std::string& sentry_crash_handler_path,
               const std::string& sentry_dsn) : spdlog::sinks::base_sink<Mutex>()
    {
        sentry_options_t* options = sentry_options_new();

        int status = -1;

        if (options != nullptr)
        {
            sentry_options_set_handler_path(options, sentry_crash_handler_path.c_str());
            sentry_options_set_dsn(options, sentry_dsn.c_str());
            sentry_options_set_database_path(options, "/tmp/.sentry-native-elk-sushi");
            status = sentry_init(options);
        }

        if (status != 0)
        {
            SUSHI_LOG_DEBUG("sentry initialization failed. "
                            "This is usually either because it lacks write access in the database path, "
                            "or because it hasn't received a valid path to the crashpad_handler executable.");
        }
    }

    ~SentrySink()
    {
        sentry_close();
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