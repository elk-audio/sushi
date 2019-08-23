/**
 * @brief Fifo queues for RtEvents
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_REALTIME_FIFO_H
#define SUSHI_REALTIME_FIFO_H

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "library/simple_fifo.h"
#include "library/rt_event.h"
#include "library/rt_event_pipe.h"

namespace sushi {

constexpr int MAX_EVENTS_IN_QUEUE = 100;

// TODO rename to RtSafeEventFifo
/**
 * @brief Wait free fifo queue for communication between rt and non-rt code
 */
class RtSafeRtEventFifo : public RtEventPipe
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

/**
 * @brief A simple RtEvent fifo implementation with internal storage that can be used
 *        internally when concurrent access from multiple threads is not neccesary
 * @tparam size Number of events to store in the queue
 */
template <size_t size>
class RtEventFifo : public SimpleFifo<RtEvent, size>, public RtEventPipe
{
public:
    RtEventFifo() = default;
    virtual ~RtEventFifo() = default;

    void send_event(const RtEvent &event) override {SimpleFifo<RtEvent, size>::push(event);}
};

} //end namespace sushi

#endif //SUSHI_REALTIME_FIFO_H
