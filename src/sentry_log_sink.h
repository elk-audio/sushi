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

namespace elk {

template<typename Mutex>
class SentrySink : public spdlog::sinks::base_sink<Mutex>
{
public:
    SentrySink() : spdlog::sinks::base_sink<Mutex>() { std::cout << "started sink" << std::endl; }
    explicit SentrySink(std::unique_ptr<spdlog::formatter> formatter) : spdlog::sinks::base_sink<Mutex>(formatter) {std::cout << "started sink" << std::endl; }
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {

    // log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
    // msg.raw contains pre formatted log

    // If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    std::cout << fmt::to_string(formatted);
    }

    void flush_() override
    {
       std::cout << std::flush;
    }
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using sentry_sink_mt = SentrySink<std::mutex>;
using sentry_sink_st = SentrySink<spdlog::details::null_mutex>;

}

#endif // SUSHI_SENTRY_LOG_SINK_H