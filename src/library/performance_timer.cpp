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
 * @brief For measuring processing performance
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <vector>

#include "performance_timer.h"
#include "logging.h"


namespace sushi {
namespace performance {

constexpr auto EVALUATION_INTERVAL = std::chrono::seconds(1);
constexpr double SEC_TO_NANOSEC = 1'000'000'000.0;
constexpr float AVERAGING_FACTOR = 0.5f;

PerformanceTimer::~PerformanceTimer()
{
    if (_enabled.load() == true)
    {
        this->enable(false);
    }
}

void PerformanceTimer::set_timing_period(TimePoint timing_period)
{
    _period = static_cast<float>(timing_period.count());
}

void PerformanceTimer::set_timing_period(float samplerate, int buffer_size)
{
    _period = static_cast<double>(buffer_size) / samplerate * SEC_TO_NANOSEC;
}

std::optional<ProcessTimings> PerformanceTimer::timings_for_node(int id)
{
    std::unique_lock<std::mutex> lock(_timing_lock);
    const auto& node = _timings.find(id);
    if (node != _timings.end())
    {
        return node->second.timings;
    }
    return std::nullopt;
}

void PerformanceTimer::enable(bool enabled)
{
    if (enabled && _enabled == false)
    {
        _enabled = true;
        _process_thread = std::thread(&PerformanceTimer::_worker, this);
    }
    else if (!enabled && _enabled == true)
    {
        _enabled = false;
        if (_process_thread.joinable())
        {
            _process_thread.join();
        }
        // Run once to clear all records
        _update_timings();
    }
}

bool PerformanceTimer::enabled()
{
    return _enabled;
}

void PerformanceTimer::_worker()
{
    while(_enabled.load())
    {
        auto start_time = std::chrono::system_clock::now();
        this->_update_timings();
        std::this_thread::sleep_until(start_time + EVALUATION_INTERVAL);
    }
}

void PerformanceTimer::_update_timings()
{
    std::map<int, std::vector<TimingLogPoint>> sorted_data;
    TimingLogPoint log_point;
    while (_entry_queue.pop(log_point))
    {
        sorted_data[log_point.id].push_back(log_point);
    }
    for (const auto& node : sorted_data)
    {
        int id = node.first;
        std::lock_guard<std::mutex> lock(_timing_lock);
        const auto& timings = _timings[id];
        auto new_timings = _calculate_timings(node.second);
        _timings[id].timings = _merge_timings(timings.timings, new_timings);
    }
}

ProcessTimings PerformanceTimer::_calculate_timings(const std::vector<TimingLogPoint>& entries) const
{
    float min_value{100};
    float max_value{0};
    float sum{0.0f};
    for (const auto& entry : entries)
    {
        float process_time = static_cast<float>(entry.delta_time.count()) / _period;
        sum += process_time;
        min_value = std::min(min_value, process_time);
        max_value = std::max(max_value, process_time);
    }
    return {sum / entries.size(), min_value, max_value};
}

ProcessTimings PerformanceTimer::_merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings)
{
    if (prev_timings.avg_case == 0.0f)
    {
        prev_timings.avg_case = new_timings.avg_case;
    }
    else
    {
        prev_timings.avg_case = (1.0f - AVERAGING_FACTOR) * prev_timings.avg_case + AVERAGING_FACTOR * new_timings.avg_case;
    }
    prev_timings.min_case = std::min(prev_timings.min_case, new_timings.min_case);
    prev_timings.max_case = std::max(prev_timings.max_case, new_timings.max_case);
    return prev_timings;
}

bool PerformanceTimer::clear_timings_for_node(int id)
{
    std::lock_guard<std::mutex> lock(_timing_lock);
    const auto& node = _timings.find(id);
    if (node != _timings.end())
    {
        new (&node->second.timings) (ProcessTimings);
        return true;
    }
    return false;
}

void PerformanceTimer::clear_all_timings()
{
    std::lock_guard<std::mutex> lock(_timing_lock);
    for (auto& node : _timings)
    {
        new (&node.second.timings) (ProcessTimings);
    }
}

} // namespace performance
} // namespace sushi