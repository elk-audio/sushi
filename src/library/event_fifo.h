/**
 * @brief Wait free fifo queue for communation between rt and non-rt code
 * @copyright MIND Music Labs AB, Stockholm
 *
 * The purpose of this is simply to be a wrapper/adapter so that we
 * eventually can replace the ableton queue with less hassle and avoid
 * littering our code with references to ableton::link::
 */

#ifndef SUSHI_REALTIME_FIFO_H
#define SUSHI_REALTIME_FIFO_H

#include "library/circular_fifo.h"
#include "library/plugin_events.h"

namespace sushi {

constexpr int MAX_EVENTS_IN_QUEUE = 100;

class EventFifo
{
public:

    inline bool push(BaseEvent* event) {return _fifo.push(event);}

    inline BaseEvent* pop()
    {
        auto item = _fifo.pop();
        return item.valid ? item.item : nullptr;
    }

    inline bool empty() {return _fifo.isEmpty();}

private:
    ableton::link::CircularFifo<BaseEvent*, MAX_EVENTS_IN_QUEUE> _fifo;
};

} //end namespace sushi

#endif //SUSHI_REALTIME_FIFO_H
