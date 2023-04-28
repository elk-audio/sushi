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

#ifndef SUSHI_PERFORMANCE_TIMER_H
#define SUSHI_PERFORMANCE_TIMER_H

#include <chrono>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>
#include <vector>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "twine/twine.h"

#include "base_performance_timer.h"
#include "constants.h"
#include "spinlock.h"

namespace sushi {
namespace performance {

using TimePoint = std::chrono::nanoseconds;
constexpr int MAX_LOG_ENTRIES = 20000;


class PerformanceTimer : public BasePerformanceTimer
{
public:
    SUSHI_DECLARE_NON_COPYABLE(PerformanceTimer);

    PerformanceTimer() = default;
    ~PerformanceTimer() override;

    /**
     * @brief Set the period to use for timings
     * @param timing_period The timing period in nanoseconds
     */
    void set_timing_period(TimePoint timing_period) override;

    /**
     * @brief Ser the period to use for timings implicitly
     * @param samplerate The samplerate in Hz
     * @param buffer_size The audio buffer size in samples
     */
    void set_timing_period(float samplerate, int buffer_size) override;

    /**
     * @brief Entry point for timing section
     * @return A timestamp representing the start of the timing period
     */
    TimePoint start_timer()
    {
        if (_enabled)
        {
            return twine::current_rt_time();
        }
        return std::chrono::nanoseconds(0);
    }

    /**
     * @brief Exit point for timing section.
     * @param start_time A timestamp from a previous call to start_timer()
     * @param node_id An integer id to identify timings from this node
     */
    void stop_timer(TimePoint start_time, int node_id)
    {
        if (_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _entry_queue.push(tp);
            // if queue is full, drop entries silently.
        }
    }

    /**
     * @brief Exit point for timing section. Safe to call concurrently from
     *       several threads.
     * @param start_time A timestamp from a previous call to start_timer()
     * @param node_id An integer id to identify timings from this node
     */
    void stop_timer_rt_safe(TimePoint start_time, int node_id)
    {
        if (_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _queue_lock.lock();
            _entry_queue.push(tp);
            _queue_lock.unlock();
            // if queue is full, drop entries silently.
        }
    }

    /**
     * @brief Enable or disable timings
     * @param enabled Enable timings if true, disable if false
     */
    void enable(bool enabled) override;

    /**
     * @brief Query the enabled state
     * @return True if the timer is enabled, false otherwise
     */
    bool enabled() override;

    /**
     * @brief Get the recorded timings from a specific node
     * @param id An integer id representing a timing node
     * @return A ProcessTimings object populated if the node has any timing records. Empty otherwise
     */
    std::optional<ProcessTimings> timings_for_node(int id) override;

    /**
     * @brief Clear the recorded timings for a particular node
     * @param id An integer id representing a timing node
     * @return true if the node was found, false otherwise
     */
    bool clear_timings_for_node(int id) override;

    /**
     * @brief Reset all recorded timings
     */
    void clear_all_timings() override;

protected:

    struct TimingLogPoint
    {
        int id {0};
        TimePoint delta_time;
    };

    struct TimingNode
    {
        int id {0};
        ProcessTimings timings;
    };

    void _worker();
    void _update_timings();

    ProcessTimings _calculate_timings(const std::vector<TimingLogPoint>& entries) const;
    static ProcessTimings _merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings);

    std::thread _process_thread;
    float _period{0};
    std::atomic_bool _enabled{false};

    std::map<int, TimingNode>  _timings;
    std::mutex _timing_lock;
    SpinLock _queue_lock;
    alignas(ASSUMED_CACHE_LINE_SIZE) memory_relaxed_aquire_release::CircularFifo<TimingLogPoint, MAX_LOG_ENTRIES> _entry_queue;
};

} // namespace performance
} // namespace sushi

#endif //SUSHI_PERFORMANCE_TIMER_H
