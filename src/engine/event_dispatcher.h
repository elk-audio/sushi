/**
 * @Brief Implementation of Event Dispatcher
 *
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_EVENT_DISPATCHER_H
#define SUSHI_EVENT_DISPATCHER_H

#include <deque>
#include <vector>
#include <thread>

#include "engine/base_event_dispatcher.h"
#include "engine/base_engine.h"
#include "engine/event_timer.h"
#include "library/synchronised_fifo.h"
#include "library/rt_event_fifo.h"
#include "library/event_interface.h"

namespace sushi {
namespace engine {class BaseEngine;}
namespace dispatcher {

class BaseEventDispatcher;

constexpr int AUDIO_ENGINE_ID = 0;
constexpr std::chrono::milliseconds THREAD_PERIODICITY = std::chrono::milliseconds(1);
constexpr auto WORKER_THREAD_PERIODICITY = std::chrono::milliseconds(1);

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

class EventDispatcher : public BaseEventDispatcher
{
public:
    EventDispatcher(engine::BaseEngine* engine, RtSafeRtEventFifo* in_rt_queue,  RtSafeRtEventFifo* out_rt_queue);

    virtual ~EventDispatcher() = default;

    void run() override;
    void stop() override;

    void post_event(Event* event) override;

    EventDispatcherStatus register_poster(EventPoster* poster) override;
    EventDispatcherStatus subscribe_to_keyboard_events(EventPoster* receiver) override;
    EventDispatcherStatus subscribe_to_parameter_change_notifications(EventPoster* receiver) override;
    EventDispatcherStatus subscribe_to_engine_notifications(EventPoster* receiver) override;
    EventDispatcherStatus deregister_poster(EventPoster* poster) override;
    EventDispatcherStatus unsubscribe_from_keyboard_events(EventPoster* receiver) override;
    EventDispatcherStatus unsubscribe_from_parameter_change_notifications(EventPoster* receiver) override;
    EventDispatcherStatus unsubscribe_from_engine_notifications(EventPoster* receiver) override;

    void set_sample_rate(float sample_rate) override {_event_timer.set_sample_rate(sample_rate);}
    void set_time(Time timestamp) override {_event_timer.set_incoming_time(timestamp);}

    int process(Event* event) override;
    int poster_id() override {return AUDIO_ENGINE_ID;}

private:

    void _event_loop();

    int _process_rt_event(RtEvent& rt_event);

    Event* _next_event();

    void _publish_keyboard_events(Event* event);
    void _publish_parameter_events(Event* event);
    void _publish_engine_notification_events(Event* event);

    std::atomic<bool>           _running;
    std::thread                 _event_thread;

    engine::BaseEngine*         _engine;

    SynchronizedQueue<Event*>   _in_queue;
    RtSafeRtEventFifo*                _in_rt_queue;
    RtSafeRtEventFifo*                _out_rt_queue;
    std::deque<Event*>          _waiting_list;

    Worker                      _worker;
    event_timer::EventTimer     _event_timer;

    std::array<EventPoster*, EventPosterId::MAX_POSTERS> _posters;
    std::vector<EventPoster*> _keyboard_event_listeners;
    std::vector<EventPoster*> _parameter_change_listeners;
    std::vector<EventPoster*> _engine_notification_listeners;
};

} // end namespace dispatcher
} // end namespace sushi

#endif //SUSHI_EVENT_DISPATCHER_H
