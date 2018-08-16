#ifndef SUSHI_HOST_CONTROL_H
#define SUSHI_HOST_CONTROL_H

#include "base_event_dispatcher.h"
#include "engine/transport.h"

namespace sushi {

/**
 * @brief Object passed to processors to allow them access to the engine
 *        to query things like time, tempo and to post non rt events.
 */
class HostControl
{
public:
    HostControl(dispatcher::BaseEventDispatcher* event_dispatcher, engine::Transport* transport) : _event_dispatcher(event_dispatcher),
                                                                                                   _transport(transport)
    {}

    void post_event(Event* event) {_event_dispatcher->post_event(event);}

    const engine::Transport* transport() {return _transport;}

protected:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    engine::Transport*               _transport;
};

} // end namespace sushi
#endif //SUSHI_HOST_CONTROL_H
