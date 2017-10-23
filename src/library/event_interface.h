/**
 * @Brief Abstract classes for adding listening and notification functionality
 *        to a class.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */


#ifndef SUSHI_EVENT_INTERFACE_H
#define SUSHI_EVENT_INTERFACE_H

#include <list>

#include "event.h"

namespace sushi {

class EventPoster
{
public:
    /**
     * @brief Function called when the poster receives an event
     * @param event The event received
     * @return An EventStatus or int code signaling how the event was handled.
     *         This will be returned to the completion callback, if the event
     *         does not have a completion callback, the return value will be
     *         ignored
     */
    virtual int process(Event* /*event*/) {return EventStatus::UNRECOGNIZED_TYPE;};

    /**
     * @brief The unique id of this poster, returned from when registering the
     *        poster with the EventDispatcher
     * @return
     */
    virtual int poster_id() = 0;

    /**
     * @brief The name of this poster
     * @return
     */
    virtual const std::string& poster_name() = 0;
};


} // end namespace sushi

#endif //SUSHI_EVENT_INTERFACE_H
