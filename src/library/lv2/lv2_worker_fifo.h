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
 * @brief Fifo queue for LV2 Worker thread requests and responses
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_WORKER_FIFO_H
#define SUSHI_LV2_WORKER_FIFO_H

#ifdef SUSHI_BUILD_WITH_LV2

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "library/simple_fifo.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"

namespace sushi {
namespace lv2 {

constexpr int MAX_ITEMS_IN_QUEUE = 128;

struct Lv2FifoItem
{
    uint32_t size{0}; // Change to int later

    // The zix ring buffer does not enforce a fixed block size.
    // Instead it is 4096 bytes in total for the buffer.
    // Each entry is a size uint, followed by as many bytes as defined in that.
    // So the safest thing would be 4096-4 really.
    //std::byte block[64]; // Old style.
    std::array<std::byte, 64> block;
};

/**
 * @brief Wait free fifo queue for communication between rt and non-rt code.
 * For use in the LV2 Worker thread implementation.
 */
class Lv2WorkerFifo
{
public:
    inline bool push(const Lv2FifoItem& item)
    {
        return _fifo.push(item);
    }

    inline bool pop(Lv2FifoItem& item)
    {
        return _fifo.pop(item);
    }

    inline bool empty()
    {
        return _fifo.wasEmpty();
    }

private:
    memory_relaxed_aquire_release::CircularFifo<Lv2FifoItem, MAX_ITEMS_IN_QUEUE> _fifo;
};

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_WORKER_FIFO_H
