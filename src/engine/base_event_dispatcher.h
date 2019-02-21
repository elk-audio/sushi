/**
 * @Brief Main event handler in Sushi and responsible for conversion between
 *        regular and rt events.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_BASE_EVENT_DISPATCHER_H
#define SUSHI_BASE_EVENT_DISPATCHER_H

#include "library/event.h"
#include "library/event_interface.h"


namespace sushi {
namespace dispatcher {

enum class EventDispatcherStatus
{
    OK,
    ALREADY_SUBSCRIBED,
    UNKNOWN_POSTER
};

/* Abstract base class is solely for test mockups */
class BaseEventDispatcher : public EventPoster
{
public:
    virtual ~BaseEventDispatcher() = default;

    virtual void run() {}
    virtual void stop() {}

    virtual void post_event(Event* event) = 0;

    virtual EventDispatcherStatus register_poster(EventPoster* /*poster*/) {return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* /*receiver*/) {return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus subscribe_to_engine_notifications(EventPoster* /*receiver*/) {return EventDispatcherStatus::OK;}

    virtual EventDispatcherStatus deregister_poster(EventPoster* /*poster*/) {return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus unsubscribe_from_keyboard_events(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus unsubscribe_from_parameter_change_notifications(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus unsubscribe_from_engine_notifications(EventPoster* /*receiver*/) {return EventDispatcherStatus::OK;}

    virtual void set_sample_rate(float /*sample_rate*/) {}
    virtual void set_time(Time /*timestamp*/) {}
};


} // end namespace dispatcher
} // end namespace sushi

#endif //SUSHI_BASE_EVENT_DISPATCHER_H
