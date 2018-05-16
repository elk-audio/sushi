#ifndef SUSHI_TIME_H
#define SUSHI_TIME_H

#include <chrono>

#include "sys/time.h"

namespace sushi {

/**
 * @brief Type used for timestamps with micro second granularity
 */
typedef std::chrono::microseconds Time;


/**
 * @brief Convenience shorthand for setting timestamp to 0, i.e. process event without delay.
 */
constexpr Time IMMEDIATE_PROCESS = std::chrono::microseconds(0);

/**
 * @brief Get the current time, only for calling from the non-rt part.
 * @return A Time object containing the current time
 */
inline Time get_current_time()
{
    timespec tp;
    int res = clock_gettime(CLOCK_REALTIME, &tp);
    if (res != 0)
    {
        return IMMEDIATE_PROCESS;
    }
    return std::chrono::seconds(tp.tv_sec) + std::chrono::duration_cast<Time>(std::chrono::nanoseconds(tp.tv_nsec));
}

} // end namespace sushi

#endif //SUSHI_TIME_H
