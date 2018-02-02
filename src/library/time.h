#ifndef SUSHI_TIME_H
#define SUSHI_TIME_H

#include <chrono>

#include "sys/time.h"

namespace sushi {

/**
 * @brief Type used for timestamps with micro second granularity
 */
//typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds> Time;
typedef std::chrono::microseconds Time;


/**
 * @brief Convenience shorthand for setting timestamp to 0, i.e. process event without delay.
 */
constexpr Time IMMEDIATE_PROCESS = std::chrono::microseconds(0);

// Maybe we're gonna need 2 of these, 1 for the rt_part and 1 for not rt if we run xenomai
// Or use it only from the non-rt and have the audio frontend deal with it and never get
// the time in the rt part.
/**
 * @brief Get the current time
 * @return A Time object containing the current time
 */
inline Time get_current_time()
{
    timespec tp;
    int res = clock_gettime(CLOCK_MONOTONIC, &tp);
    if (res != 0)
    {
        return IMMEDIATE_PROCESS;
    }
    return std::chrono::seconds(tp.tv_sec) + std::chrono::duration_cast<Time>(std::chrono::nanoseconds(tp.tv_nsec));
}

} // end namespace sushi

#endif //SUSHI_TIME_H
