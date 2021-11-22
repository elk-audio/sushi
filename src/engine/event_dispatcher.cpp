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
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "event_dispatcher.h"
#include "engine/base_engine.h"
#include "logging.h"

namespace sushi {
namespace dispatcher {

constexpr auto TIMING_UPDATE_INTERVAL = std::chrono::seconds(1);

// SUSHI_GET_LOGGER_WITH_MODULE_NAME("event dispatcher"); TODO: No logging causes unused variable warning in clang

EventDispatcher::EventDispatcher(engine::BaseEngine* engine,
                                 RtSafeRtEventFifo* in_rt_queue,
                                 RtSafeRtEventFifo* out_rt_queue) : _running{false},
                                                                    /* _engine{engine}, TODO: Unused according to clang*/
                                                                    _in_rt_queue{in_rt_queue},
                                                                    _out_rt_queue{out_rt_queue},
                                                                    _worker{engine, this},
                                                                    _event_timer{engine->sample_rate()}
{
    std::fill(_posters.begin(), _posters.end(), nullptr);
    register_poster(this);
    register_poster(&_worker);
}

void EventDispatcher::post_event(Event* event)
{
    _in_queue.push(event);
}

EventDispatcherStatus EventDispatcher::register_poster(EventPoster* poster)
{
    if (_posters[poster->poster_id()] != nullptr)
    {
        return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _posters[poster->poster_id()] = poster;
    return EventDispatcherStatus::OK;
}

void EventDispatcher::run()
{
    if (_running == false)
    {
        _running = true;
        _event_thread = std::thread(&EventDispatcher::_event_loop, this);
        _worker.run();
    }
}

void EventDispatcher::stop()
{
    _running = false;
    _worker.stop();
    if (_event_thread.joinable())
    {
        _event_thread.join();
    }
}

EventDispatcherStatus EventDispatcher::subscribe_to_keyboard_events(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_keyboard_listener_lock);

    for (auto r : _keyboard_event_listeners)
    {
        if (r == receiver) return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _keyboard_event_listeners.push_back(receiver);
    return EventDispatcherStatus::OK;
}

EventDispatcherStatus EventDispatcher::subscribe_to_parameter_change_notifications(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_parameter_listener_lock);

    for (auto r : _parameter_change_listeners)
    {
        if (r == receiver) return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _parameter_change_listeners.push_back(receiver);
    return EventDispatcherStatus::OK;
}

EventDispatcherStatus EventDispatcher::subscribe_to_engine_notifications(EventPoster*receiver)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto r : _engine_notification_listeners)
    {
        if (r == receiver) return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _engine_notification_listeners.push_back(receiver);
    return EventDispatcherStatus::OK;
}

int EventDispatcher::process(Event* event)
{
    if (event->process_asynchronously())
    {
        event->set_receiver(EventPosterId::WORKER);
        return _posters[event->receiver()]->process(event);
    }
    if (event->maps_to_rt_event())
    {
        auto [send_now, sample_offset] = _event_timer.sample_offset_from_realtime(event->time());
        if (send_now)
        {
            if (_out_rt_queue->push(event->to_rt_event(sample_offset)))
            {
                return EventStatus::HANDLED_OK;
            }
        }
        _waiting_list.push_front(event);
        return EventStatus::QUEUED_HANDLING;
    }
    if (event->is_parameter_change_notification())
    {
        _publish_parameter_events(event);
        return EventStatus::HANDLED_OK;
    }
    if (event->is_engine_notification())
    {
        _publish_engine_notification_events(event);
        return EventStatus::HANDLED_OK;
    }
    return EventStatus::UNRECOGNIZED_EVENT;
}

void EventDispatcher::_event_loop()
{
    do
    {
        auto start_time = std::chrono::system_clock::now();

        /* Handle incoming Events */
        while (Event* event = _next_event())
        {
            assert(event->receiver() < static_cast<int>(_posters.size()));
            EventPoster* receiver = _posters[event->receiver()];
            int status = EventStatus::UNRECOGNIZED_RECEIVER;
            if (receiver != nullptr)
            {
                status = _posters[event->receiver()]->process(event);
            }
            if (status == EventStatus::QUEUED_HANDLING)
            {
                /* Event has not finished processing, so dont call comp cb or delete it */
                continue;
            }
            if (event->completion_cb() != nullptr)
            {
                event->completion_cb()(event->callback_arg(), event, status);
            }
            delete(event);
        }
        /* Handle incoming RtEvents */
        while (!_in_rt_queue->empty())
        {
            RtEvent rt_event;
            _in_rt_queue->pop(rt_event);
            _process_rt_event(rt_event);
        }
        std::this_thread::sleep_until(start_time + THREAD_PERIODICITY);
    }
    while (_running);
}

int EventDispatcher::_process_rt_event(RtEvent &rt_event)
{
    Time timestamp = _event_timer.real_time_from_sample_offset(rt_event.sample_offset());
    Event* event = Event::from_rt_event(rt_event, timestamp);
    if (event == nullptr)
    {
        switch (rt_event.type())
        {
            case RtEventType::SYNC:
            {
                auto typed_event = rt_event.syncronisation_event();
                _event_timer.set_outgoing_time(typed_event->timestamp());
                return EventStatus::HANDLED_OK;
            }
            default:
                return EventStatus::UNRECOGNIZED_EVENT;
        }
    }
    if (event->is_keyboard_event())
    {
        _publish_keyboard_events(event);
    }
    if (event->is_parameter_change_notification())
    {
        _publish_parameter_events(event);
    }
    if (event->is_engine_notification())
    {
        _publish_engine_notification_events(event);
    }
    if (event->process_asynchronously())
    {
        return _worker.process(event);
    }
    // TODO - better lifetime management of events
    delete event;
    return EventStatus::HANDLED_OK;
}

Event*EventDispatcher::_next_event()
{
    Event* event = nullptr;
    if (!_waiting_list.empty())
    {
        event = _waiting_list.back();
        _waiting_list.pop_back();
    }
    else if (!_in_queue.empty())
    {
        event = _in_queue.pop();
    }
    return event;
}

void EventDispatcher::_publish_keyboard_events(Event* event)
{
    std::lock_guard<std::mutex> lock(_keyboard_listener_lock);

    for (auto& listener : _keyboard_event_listeners)
    {
        listener->process(event);
    }
}

void EventDispatcher::_publish_parameter_events(Event* event)
{
    std::lock_guard<std::mutex> lock(_parameter_listener_lock);

    for (auto& listener : _parameter_change_listeners)
    {
        listener->process(event);
    }
}

void EventDispatcher::_publish_engine_notification_events(sushi::Event* event)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto& listener : _engine_notification_listeners)
    {
        listener->process(event);
    }
}

EventDispatcherStatus EventDispatcher::deregister_poster(EventPoster* poster)
{
    if (_posters[poster->poster_id()] != nullptr)
    {
        _posters[poster->poster_id()] = nullptr;
        return EventDispatcherStatus::OK;
    }
    return EventDispatcherStatus::UNKNOWN_POSTER;
}

EventDispatcherStatus EventDispatcher::unsubscribe_from_keyboard_events(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_keyboard_listener_lock);

    for (auto i = _keyboard_event_listeners.begin(); i != _keyboard_event_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _keyboard_event_listeners.erase(i);
            return EventDispatcherStatus::OK;
        }
    }
    return EventDispatcherStatus::UNKNOWN_POSTER;
}

EventDispatcherStatus EventDispatcher::unsubscribe_from_parameter_change_notifications(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_parameter_listener_lock);

    for (auto i = _parameter_change_listeners.begin(); i != _parameter_change_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _parameter_change_listeners.erase(i);
            return EventDispatcherStatus::OK;
        }
    }
    return EventDispatcherStatus::UNKNOWN_POSTER;
}

EventDispatcherStatus EventDispatcher::unsubscribe_from_engine_notifications(EventPoster*receiver)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto i = _engine_notification_listeners.begin(); i != _engine_notification_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _engine_notification_listeners.erase(i);
            return EventDispatcherStatus::OK;
        }
    }
    return EventDispatcherStatus::UNKNOWN_POSTER;
}

void Worker::run()
{
    _running = true;
    _worker_thread = std::thread(&Worker::_worker, this);
}

void Worker::stop()
{
    _running = false;
    if (_worker_thread.joinable())
    {
        _worker_thread.join();
    }
}

int Worker::process(Event*event)
{
    _queue.push(event);
    return EventStatus::QUEUED_HANDLING;
}

void Worker::_worker()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> timing_update_counter;
    do
    {
        auto start_time = std::chrono::system_clock::now();
        while (!_queue.empty())
        {
            int status = EventStatus::UNRECOGNIZED_EVENT;
            Event* event = _queue.pop();

            if (event->is_engine_event())
            {
                auto typed_event = static_cast<EngineEvent*>(event);
                status = typed_event->execute(_engine);
            }
            if (event->is_async_work_event())
            {
                auto typed_event = static_cast<AsynchronousWorkEvent*>(event);
                Event* response_event = typed_event->execute();
                if (response_event != nullptr)
                {
                    _dispatcher->post_event(response_event);
                }
            }

            if (event->completion_cb() != nullptr)
            {
                event->completion_cb()(event->callback_arg(), event, status);
            }
            delete (event);
        }
        if (start_time > timing_update_counter + TIMING_UPDATE_INTERVAL)
        {
            timing_update_counter = start_time;
            _engine->update_timings();
        }

        std::this_thread::sleep_until(start_time + WORKER_THREAD_PERIODICITY);
    }
    while (_running);
}


} // end namespace dispatcher
} // end namespace sushi