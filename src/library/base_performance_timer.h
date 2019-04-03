/**
 * @Brief real time audio processing engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_BASE_PERFORMANCE_TIMER_H
#define SUSHI_BASE_PERFORMANCE_TIMER_H

#include <optional>

namespace sushi {
namespace performance {

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


} // namespace performance
} // namespace sushi

#endif //SUSHI_BASE_PERFORMANCE_TIMER_H
