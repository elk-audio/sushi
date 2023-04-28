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
 * @Brief Implementation of Event Dispatcher
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_EVENT_DISPATCHER_H
#define SUSHI_EVENT_DISPATCHER_H

#include <deque>
#include <vector>
#include <thread>

#include "engine/base_event_dispatcher.h"
#include "engine/base_engine.h"
#include "engine/event_timer.h"
#include "engine/parameter_manager.h"
#include "library/synchronised_fifo.h"
#include "library/rt_event_fifo.h"
#include "library/event_interface.h"

namespace sushi {
namespace engine {class BaseEngine;}
namespace dispatcher {

class BaseEventDispatcher;

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

    ~EventDispatcher() override;

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
    int poster_id() override;

private:
    void _event_loop();

    int _process_rt_event(RtEvent& rt_event);

    Event* _next_event();

    void _publish_keyboard_events(Event* event);
    void _publish_parameter_events(Event* event);
    void _publish_engine_notification_events(Event* event);
    void _handle_engine_notifications_internally(EngineNotificationEvent* event);

    std::atomic<bool>           _running;
    std::thread                 _event_thread;

    SynchronizedQueue<Event*>   _in_queue;
    RtSafeRtEventFifo*          _in_rt_queue;
    RtSafeRtEventFifo*          _out_rt_queue;
    std::deque<Event*>          _waiting_list;

    Worker                      _worker;
    event_timer::EventTimer     _event_timer;
    ParameterManager            _parameter_manager;
    int                         _parameter_update_count;
    Time                        _last_rt_event_time;

    std::array<EventPoster*, EventPosterId::MAX_POSTERS> _posters;
    std::vector<EventPoster*> _keyboard_event_listeners;
    std::vector<EventPoster*> _parameter_change_listeners;
    std::vector<EventPoster*> _engine_notification_listeners;

    std::mutex _keyboard_listener_lock;
    std::mutex _parameter_listener_lock;
    std::mutex _engine_listener_lock;
};

} // end namespace dispatcher
} // end namespace sushi

#endif //SUSHI_EVENT_DISPATCHER_H
