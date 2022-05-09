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
 * @brief Helper for asynchronous communication
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ASYNCHRONOUS_RECEIVER_H
#define SUSHI_ASYNCHRONOUS_RECEIVER_H

#include <vector>
#include <chrono>

#include "library/id_generator.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace receiver {

class AsynchronousEventReceiver
{
public:
    explicit AsynchronousEventReceiver(RtSafeRtEventFifo* queue) : _queue{queue} {}

    /**
     * @brief Blocks the current thread while waiting for a response to a given event
     * @param id EventId of the event the thread is waiting for
     * @param timeout Maximum wait time
     * @return true if the event was received in time and handled properly, false otherwise
     */
    bool wait_for_response(EventId id, std::chrono::milliseconds timeout);

private:
    struct Node
    {
        EventId id;
        bool    status;
    };
    std::vector<Node> _receive_list;
    RtSafeRtEventFifo* _queue;
};


} // end namespace receiver
} // end namespace sushi

#endif //SUSHI_ASYNCHRONOUS_RECEIVER_H
