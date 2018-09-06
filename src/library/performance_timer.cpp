#include <vector>

#include "performance_timer.h"
#include "logging.h"

MIND_GET_LOGGER_WITH_MODULE_NAME("processtimer");

namespace sushi {
namespace performance {

constexpr auto EVALUATION_INTERVAL = std::chrono::seconds(1);
constexpr double SEC_TO_NANOSEC = 1'000'000'000.0;
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
    }
}

void PerformanceTimer::_worker()
{
    while(_enabled.load())
    {
        auto start_time = std::chrono::system_clock::now();
        std::map<int, std::vector<TimingLogPoint>> sorted_data;
        TimingLogPoint log_point;
        while (_entry_queue.pop(log_point))
        {
            sorted_data[log_point.id].push_back(log_point);
        }
        for (const auto& node : sorted_data)
        {
            int id = node.first;
            std::unique_lock<std::mutex> lock(_timing_lock);
            const auto& timings = _timings[id];
            lock.unlock();
            auto new_timings = _calculate_timings(node.second);
            lock.lock();
            _timings[id].timings = _merge_timings(timings.timings, new_timings);
        }
        std::this_thread::sleep_until(start_time + EVALUATION_INTERVAL);
    }
}

ProcessTimings PerformanceTimer::_calculate_timings(const std::vector<TimingLogPoint>& entries)
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
    return {.avg_case = sum / entries.size(), .min_case = min_value, .max_case = max_value};
}

ProcessTimings PerformanceTimer::_merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings)
{
    if (prev_timings.avg_case == 0.0f)
    {
        prev_timings.avg_case = new_timings.avg_case;
    }
    else
    {
        prev_timings.avg_case = (prev_timings.avg_case + new_timings.avg_case) / 2.0f;
    }
    prev_timings.min_case = std::min(prev_timings.min_case, new_timings.min_case);
    prev_timings.max_case = std::max(prev_timings.max_case, new_timings.max_case);
    return prev_timings;
}


} // namespace performance
} // namespace sushi