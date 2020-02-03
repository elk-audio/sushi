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

#ifdef SUSHI_BUILD_WITH_LV2

#ifndef SUSHI_LV2_SEMAPHORE_H
#define SUSHI_LV2_SEMAPHORE_H

#include <mutex>
#include <condition_variable>

// This class is only used for pausing or continuing playback, so even if a mode switch occurs,
// it will coincide with audio stopping/starting.

namespace sushi {
namespace lv2 {

class Semaphore {
public:
    Semaphore (int count = 0)
            : _count(count) {}

    ~Semaphore() = default;

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _count++;
        _cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(_mtx);

        while (_count == 0)
        {
            _cv.wait(lock);
        }

        _count--;
    }

private:
    std::mutex _mtx;
    std::condition_variable _cv;
    int _count;
};

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_SEMAPHORE_H
