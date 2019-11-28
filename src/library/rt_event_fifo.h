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
 * @brief Wait free fifo queue for communication between rt and non-rt code
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_REALTIME_FIFO_H
#define SUSHI_REALTIME_FIFO_H

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"

namespace sushi {

constexpr int MAX_EVENTS_IN_QUEUE = 100;

class RtEventFifo : public RtEventPipe
{
public:

    inline bool push(RtEvent event) {return _fifo.push(event);}

    inline bool pop(RtEvent& event)
    {
        return _fifo.pop(event);
    }

    inline bool empty() {return _fifo.wasEmpty();}

    virtual void send_event(RtEvent event) override {push(event);}

private:
    memory_relaxed_aquire_release::CircularFifo<RtEvent, MAX_EVENTS_IN_QUEUE> _fifo;
};

} //end namespace sushi

#endif //SUSHI_REALTIME_FIFO_H
