/**
 * @brief Interface for events handlers/dispatchers/queues
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Abstract interface for classes that receive and pass on events
 * to other. In some sense separate from Processor since this is
 * not intended to process or consume events, only pass them on.
 */

#ifndef SUSHI_RT_EVENT_PIPE_H
#define SUSHI_RT_EVENT_PIPE_H

#include "library/rt_event.h"

namespace sushi {

class RtEventPipe
{
public:
    virtual void send_event(RtEvent event) = 0;
};

} // end namespace sushi


#endif //SUSHI_RT_EVENT_PIPE_H
