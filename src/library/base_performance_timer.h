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
 * @brief Measure processing performance
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_BASE_PERFORMANCE_TIMER_H
#define SUSHI_BASE_PERFORMANCE_TIMER_H

#include <optional>

namespace sushi::internal::performance {

struct ProcessTimings
{
    ProcessTimings() : avg_case{0.0f}, min_case{100.0f}, max_case{0.0f} {}
    ProcessTimings(float avg, float min, float max) : avg_case{avg}, min_case{min}, max_case{max} {}
    float avg_case{1};
    float min_case{1};
    float max_case{0};
};

class BasePerformanceTimer
{
public:
    BasePerformanceTimer() = default;
    virtual ~BasePerformanceTimer() = default;
    /**
    * @brief Set the period to use for timings
    * @param timing_period The timing period in nanoseconds
    */
    virtual void set_timing_period(std::chrono::nanoseconds timing_period) = 0;

    /**
     * @brief Ser the period to use for timings implicitly
     * @param samplerate The samplerate in Hz
     * @param buffer_size The audio buffer size in samples
     */
    virtual void set_timing_period(float samplerate, int buffer_size) = 0;

    /**
     * @brief Enable or disable timings
     * @param enabled Enable timings if true, disable if false
     */
    virtual void enable(bool enabled) = 0;

    /**
     * @brief Query the enabled state
     * @return True if the timer is enabled, false otherwise
     */
    virtual bool enabled() = 0;

    /**
     * @brief Get the recorded timings from a specific node
     * @param id An integer id representing a timing node
     * @return A ProcessTimings object populated if the node has any timing records. Empty otherwise
     */
    virtual std::optional<ProcessTimings> timings_for_node(int id) = 0;

    /**
     * @brief Clear the recorded timings for a particular node
     * @param id An integer id representing a timing node
     * @return true if the node was found, false otherwise
     */
    virtual bool clear_timings_for_node(int id) = 0;

    /**
     * @brief Reset all recorded timings
     */
    virtual void clear_all_timings() = 0;
};

} // end namespace sushi::internal::performance

#endif // SUSHI_BASE_PERFORMANCE_TIMER_H
