/**
 * @Brief Main event handler in Sushi and responsible for convertion between
 *        regular and rt events.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_EVENT_DISPATCHER_H
#define SUSHI_EVENT_DISPATCHER_H

#include <mutex>
#include <thread>

#include "logging.h"
#include "engine/event_timer.h"
#include "library/rt_event.h"
#include "library/synchronised_fifo.h"
#include "library/event.h"
#include "library/event_interface.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace engine {class BaseEngine;}
namespace dispatcher {

class BaseEventDispatcher;

constexpr int AUDIO_ENGINE_ID = 0;
constexpr std::chrono::milliseconds THREAD_PERIODICITY = std::chrono::milliseconds(1);
constexpr auto WORKER_THREAD_PERIODOCITY = std::chrono::milliseconds(1);

enum class EventDispatcherStatus
{
    OK,
    ALREADY_SUBSCRIBED,
    UNKNOWN_POSTER
};

/**
 * @brief Low priority worker for handling possibly time consuming tasks like
 * instantiating plugins or do asynchronous work from processors.
 */
class Worker : public EventPoster
{
public:
    Worker(engine::BaseEngine* engine, BaseEventDispatcher* dispatcher) : _engine(engine),
                                                                          _dispatcher(dispatcher),
                                                                          _running(false) {}

    virtual ~Worker() = default;

    void run();
    void stop();

    int process(Event* event) override;
    int poster_id() override {return EventPosterId::WORKER;}

private:
    engine::BaseEngine*         _engine;
    BaseEventDispatcher*        _dispatcher;

    void                        _worker();
    std::thread                 _worker_thread;
    std::atomic<bool>           _running;

    SynchronizedQueue<Event*>   _queue;
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
    virtual EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}

    virtual EventDispatcherStatus deregister_poster(EventPoster* /*poster*/) {return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus unsubscribe_from_keyboard_events(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
    virtual EventDispatcherStatus unsubscribe_from_parameter_change_notifications(EventPoster* /*receiver*/) { return EventDispatcherStatus::OK;}
};

class EventDispatcher : public BaseEventDispatcher
{
public:
    EventDispatcher(engine::BaseEngine* engine, RtEventFifo* in_rt_queue,  RtEventFifo* out_rt_queue);

    virtual ~EventDispatcher() = default;

    void run() override;
    void stop() override;

    void post_event(Event* event) override;

    EventDispatcherStatus register_poster(EventPoster* poster) override;
    EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* receiver) override;
    EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* receiver) override;
    EventDispatcherStatus deregister_poster(EventPoster* poster) override;
    EventDispatcherStatus unsubscribe_from_keyboard_events(EventPoster* receiver) override;
    EventDispatcherStatus unsubscribe_from_parameter_change_notifications(EventPoster* receiver) override;

    int process(Event* event) override;
    int poster_id() override {return AUDIO_ENGINE_ID;}

private:

    void _event_loop();

    int _process_rt_event(RtEvent& rt_event);

    void _publish_keyboard_events(Event* event);
    void _publish_parameter_events(Event* event);

    std::atomic<bool>           _running;
    std::thread                 _event_thread;

    engine::BaseEngine*         _engine;

    SynchronizedQueue<Event*>   _in_queue;
    RtEventFifo*                _in_rt_queue;
    RtEventFifo*                _out_rt_queue;

    Worker                      _worker;

    std::array<EventPoster*, EventPosterId::MAX_POSTERS> _posters;
    std::vector<EventPoster*> _keyboard_event_listeners;
    std::vector<EventPoster*> _parameter_change_listeners;
};

} // end namespace dispatcher
} // end namespace sushi

#endif //SUSHI_EVENT_DISPATCHER_H
