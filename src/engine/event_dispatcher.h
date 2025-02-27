/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @Brief Implementation of Event Dispatcher
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
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

namespace sushi::internal::engine {
    class BaseEngine;
}

namespace sushi::internal::dispatcher {

class BaseEventDispatcher;

class Accessor;
class WorkerAccessor;

using EventQueue = SynchronizedQueue<std::unique_ptr<Event>>;

/**
 * @brief Low priority worker for handling possibly time consuming tasks like
 * instantiating plugins or do asynchronous work from processors.
 */
class Worker
{
public:
    Worker(engine::BaseEngine* engine, BaseEventDispatcher* dispatcher) : _engine(engine),
                                                                          _dispatcher(dispatcher),
                                                                          _running(false) {}

    virtual ~Worker() = default;

    void run();
    void stop();

    int dispatch(std::unique_ptr<Event> event);

private:
    friend Accessor;
    friend WorkerAccessor;

    engine::BaseEngine*         _engine;
    BaseEventDispatcher*        _dispatcher;

    void                        _worker();
    std::thread                 _worker_thread;
    std::atomic<bool>           _running;

    EventQueue _queue;
};

class EventDispatcher : public BaseEventDispatcher
{
public:
    EventDispatcher(engine::BaseEngine* engine, RtSafeRtEventFifo* in_rt_queue,  RtSafeRtEventFifo* out_rt_queue);

    ~EventDispatcher() override;

    void run() override;
    void stop() override;

    void post_event(std::unique_ptr<Event> event) override;

    Status subscribe_to_keyboard_events(EventPoster* receiver) override;
    Status subscribe_to_parameter_change_notifications(EventPoster* receiver) override;
    Status subscribe_to_engine_notifications(EventPoster* receiver) override;

    Status unsubscribe_from_keyboard_events(EventPoster* receiver) override;
    Status unsubscribe_from_parameter_change_notifications(EventPoster* receiver) override;
    Status unsubscribe_from_engine_notifications(EventPoster* receiver) override;

    void set_sample_rate(float sample_rate) override {_event_timer.set_sample_rate(sample_rate);}
    void set_time(Time timestamp) override {_event_timer.set_incoming_time(timestamp);}

    int dispatch(std::unique_ptr<Event> event) override;

private:
    friend Accessor;

    void _event_loop();

    int _process_rt_event(RtEvent& rt_event);

    std::unique_ptr<Event> _next_event();

    void _publish_keyboard_events(Event* event);
    void _publish_parameter_events(Event* event);
    void _publish_engine_notification_events(Event* event);
    void _handle_engine_notifications_internally(EngineNotificationEvent* event);

    std::atomic<bool>           _running;
    std::thread                 _event_thread;

    EventQueue _in_queue;

    RtSafeRtEventFifo*          _in_rt_queue;
    RtSafeRtEventFifo*          _out_rt_queue;

    std::deque<std::unique_ptr<Event>> _waiting_list;

    Worker                      _worker;
    event_timer::EventTimer     _event_timer;
    ParameterManager            _parameter_manager;
    int                         _parameter_update_count;
    Time                        _last_rt_event_time{};

    std::vector<EventPoster*> _keyboard_event_listeners;
    std::vector<EventPoster*> _parameter_change_listeners;
    std::vector<EventPoster*> _engine_notification_listeners;

    std::mutex _keyboard_listener_lock;
    std::mutex _parameter_listener_lock;
    std::mutex _engine_listener_lock;
};

} // end namespace sushi::internal::dispatcher

#endif // SUSHI_EVENT_DISPATCHER_H
