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
 * @brief Basic spinlock implementation safe for xenomai use
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SPINLOCK_H
#define SUSHI_SPINLOCK_H

#include <atomic>

#include "constants.h"

// since std::hardware_destructive_interference_size is not yet supported in GCC 7
constexpr int ASSUMED_CACHE_LINE_SIZE = 64;

namespace sushi {
/**
 * @brief Simple rt-safe test-and-set spinlock
 */
class SpinLock
{
public:
    SpinLock() = default;

    SUSHI_DECLARE_NON_COPYABLE(SpinLock);

    void lock()
    {
        while (flag.load(std::memory_order_relaxed) == true)
        {
            /* Spin until flag is cleared. According to
             * https://geidav.wordpress.com/2016/03/23/test-and-set-spinlocks/
             * this is better as it causes fewer cache invalidations */
        }
        while (flag.exchange(true, std::memory_order_acquire))
        {}
    }

    void unlock()
    {
        flag.store(false, std::memory_order_release);
    }

private:
    alignas(ASSUMED_CACHE_LINE_SIZE) std::atomic_bool flag{false};
};

} // end namespace sushi
#endif //SUSHI_SPINLOCK_H
