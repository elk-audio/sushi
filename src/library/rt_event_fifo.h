/**
 * @brief Wait free fifo queue for communication between rt and non-rt code
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_REALTIME_FIFO_H
#define SUSHI_REALTIME_FIFO_H

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"

namespace sushi {

constexpr int MAX_EVENTS_IN_QUEUE = 100;

// TODO rename to RtSafeEventFifo
class RtEventFifo : public RtEventPipe
{
public:

    inline bool push(const RtEvent& event) {return _fifo.push(event);}

    inline bool pop(RtEvent& event)
    {
        return _fifo.pop(event);
    }

    inline bool empty() {return _fifo.wasEmpty();}

    void send_event(const RtEvent &event) override {push(event);}

private:
    memory_relaxed_aquire_release::CircularFifo<RtEvent, MAX_EVENTS_IN_QUEUE> _fifo;
};


} //end namespace sushi

#endif //SUSHI_REALTIME_FIFO_H
