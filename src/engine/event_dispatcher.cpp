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

#include "elklog/static_logger.h"

#include "event_dispatcher.h"
#include "engine/base_engine.h"

namespace sushi::internal::dispatcher {

constexpr std::chrono::milliseconds THREAD_PERIODICITY = std::chrono::milliseconds(1);
constexpr auto WORKER_THREAD_PERIODICITY = std::chrono::milliseconds(1);
constexpr auto TIMING_UPDATE_INTERVAL = std::chrono::seconds(1);
constexpr auto PARAMETER_UPDATE_RATE = 10;
// Rate limits broadcast parameter updates to 25 Hz
constexpr auto MAX_PARAMETER_UPDATE_INTERVAL = std::chrono::milliseconds(40);

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("event dispatcher");

EventDispatcher::EventDispatcher(engine::BaseEngine* engine,
                                 RtSafeRtEventFifo* in_rt_queue,
                                 RtSafeRtEventFifo* out_rt_queue) : _engine(engine),
                                                                    _running{false},
                                                                    _in_rt_queue{in_rt_queue},
                                                                    _out_rt_queue{out_rt_queue},
                                                                    _worker{engine, this},
                                                                    _event_timer{engine->sample_rate()},
                                                                    _parameter_manager{MAX_PARAMETER_UPDATE_INTERVAL,
                                                                                       engine->processor_container()},
                                                                    _parameter_update_count{0}
{}

EventDispatcher::~EventDispatcher()
{
    if (_running)
    {
        stop();
    }

    while (!_in_queue.empty())
    {
        // As each event goes out of scope, it's deleted.
        auto event = _in_queue.pop();
    }
}

void EventDispatcher::post_event(std::unique_ptr<Event> event)
{
    _in_queue.push(std::move(event));
}

void EventDispatcher::run()
{
    if (!_running)
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

Status EventDispatcher::subscribe_to_keyboard_events(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_keyboard_listener_lock);

    for (auto r : _keyboard_event_listeners)
    {
        if (r == receiver) return Status::ALREADY_SUBSCRIBED;
    }
    _keyboard_event_listeners.push_back(receiver);
    return Status::OK;
}

Status EventDispatcher::subscribe_to_parameter_change_notifications(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_parameter_listener_lock);

    for (auto r : _parameter_change_listeners)
    {
        if (r == receiver) return Status::ALREADY_SUBSCRIBED;
    }
    _parameter_change_listeners.push_back(receiver);
    return Status::OK;
}

Status EventDispatcher::subscribe_to_engine_notifications(EventPoster*receiver)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto r : _engine_notification_listeners)
    {
        if (r == receiver) return Status::ALREADY_SUBSCRIBED;
    }
    _engine_notification_listeners.push_back(receiver);
    return Status::OK;
}

int EventDispatcher::dispatch(std::unique_ptr<Event> event)
{
    int status = EventStatus::NOT_HANDLED;

    if (event->process_asynchronously())
    {
        return _worker.dispatch(std::move(event));
    }

    if (event->is_parameter_change_event())
    {
        auto typed_event = static_cast<const ParameterChangeEvent*>(event.get());
        _parameter_manager.mark_parameter_changed(typed_event->processor_id(),
                                                  typed_event->parameter_id(),
                                                  typed_event->time());
    }

    if (event->maps_to_rt_event())
    {
        auto [send_now, sample_offset] = _event_timer.sample_offset_from_realtime(event->time());
        if (send_now)
        {
            if (_out_rt_queue->push(event->to_rt_event(sample_offset)))
            {
                status = EventStatus::HANDLED_OK;
            }
        }
        else
        {
            _waiting_list.push_front(std::move(event));

            // Dispatch will be called with this event again. When it HAS run, callback is called.
            return EventStatus::QUEUED_HANDLING;
        }
    }

    if (event->is_parameter_change_notification() || event->is_property_change_notification())
    {
        _publish_parameter_events(event.get());
        status = EventStatus::HANDLED_OK;
    }

    if (event->is_engine_notification())
    {
        _handle_engine_notifications_internally(static_cast<EngineNotificationEvent*>(event.get()));
        _publish_engine_notification_events(event.get());
        status = EventStatus::HANDLED_OK;
    }

    if (status == EventStatus::HANDLED_OK)
    {
        if (event->completion_cb() != nullptr)
        {
            event->completion_cb()(event->callback_arg(), event.get(), status);
        }
    }
    else
    {
        ELKLOG_LOG_ERROR("There should never be an unrecognized event.");
        // If there is one, the above event handling chain is broken.

        assert(false);

        status = EventStatus::UNRECOGNIZED_EVENT;
    }

    return status;
}

void EventDispatcher::_event_loop()
{
    do
    {
        auto start_time = std::chrono::system_clock::now();

        // Handle incoming Events
        while (auto event = _next_event())
        {
            dispatch(std::move(event));
        }

        // Handle incoming RtEvents
        while (!_in_rt_queue->empty())
        {
            RtEvent rt_event;
            _in_rt_queue->pop(rt_event);
            _process_rt_event(rt_event);
        }

        // Send updates for any parameters that have changed
        if (_parameter_update_count++ >= PARAMETER_UPDATE_RATE)
        {
            _parameter_manager.output_parameter_notifications(this, _last_rt_event_time);
            _parameter_update_count = 0;
        }

        if (!_engine->realtime())
        {
            _engine->clear_rt_queues();
        }

        std::this_thread::sleep_until(start_time + THREAD_PERIODICITY);
    }
    while (_running);
}

int EventDispatcher::_process_rt_event(RtEvent &rt_event)
{
    if (rt_event.type() == RtEventType::FLOAT_PARAMETER_CHANGE ||
        rt_event.type() == RtEventType::INT_PARAMETER_CHANGE ||
        rt_event.type() == RtEventType::BOOL_PARAMETER_CHANGE)
    {
        auto typed_event = rt_event.parameter_change_event();
        _parameter_manager.mark_parameter_changed(typed_event->processor_id(), typed_event->param_id(), IMMEDIATE_PROCESS);
        return EventStatus::HANDLED_OK;
    }

    Time timestamp = _event_timer.real_time_from_sample_offset(rt_event.sample_offset());
    auto event = Event::from_rt_event(rt_event, timestamp);
    if (event == nullptr)
    {
        switch (rt_event.type())
        {
            case RtEventType::SYNC:
            {
                auto typed_event = rt_event.syncronisation_event();
                _event_timer.set_outgoing_time(typed_event->timestamp());
                _last_rt_event_time = typed_event->timestamp();
                return EventStatus::HANDLED_OK;
            }

            default:
                return EventStatus::UNRECOGNIZED_EVENT;
        }
    }

    if (event->is_keyboard_event())
    {
        _publish_keyboard_events(event.get());
    }
    else if (event->is_engine_notification())
    {
        _publish_engine_notification_events(event.get());
    }

    if (event->process_asynchronously())
    {
        return _worker.dispatch(std::move(event));
    }

    return EventStatus::HANDLED_OK;
}

std::unique_ptr<Event> EventDispatcher::_next_event()
{
    std::unique_ptr<Event> event = nullptr;
    if (!_waiting_list.empty())
    {
        event = std::move(_waiting_list.back());
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

void EventDispatcher::_publish_engine_notification_events(Event* event)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto& listener : _engine_notification_listeners)
    {
        listener->process(event);
    }
}

Status EventDispatcher::unsubscribe_from_keyboard_events(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_keyboard_listener_lock);

    for (auto i = _keyboard_event_listeners.begin(); i != _keyboard_event_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _keyboard_event_listeners.erase(i);
            return Status::OK;
        }
    }
    return Status::UNKNOWN_POSTER;
}

Status EventDispatcher::unsubscribe_from_parameter_change_notifications(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_parameter_listener_lock);

    for (auto i = _parameter_change_listeners.begin(); i != _parameter_change_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _parameter_change_listeners.erase(i);
            return Status::OK;
        }
    }
    return Status::UNKNOWN_POSTER;
}

Status EventDispatcher::unsubscribe_from_engine_notifications(EventPoster* receiver)
{
    std::lock_guard<std::mutex> lock(_engine_listener_lock);

    for (auto i = _engine_notification_listeners.begin(); i != _engine_notification_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _engine_notification_listeners.erase(i);
            return Status::OK;
        }
    }
    return Status::UNKNOWN_POSTER;
}

void EventDispatcher::_handle_engine_notifications_internally(EngineNotificationEvent* event)
{
    if (event->is_audio_graph_notification())
    {
        auto typed_event = static_cast<AudioGraphNotificationEvent*>(event);
        switch (typed_event->action())
        {
            case AudioGraphNotificationEvent::Action::PROCESSOR_CREATED:
                _parameter_manager.track_parameters(typed_event->processor());
                break;

            case AudioGraphNotificationEvent::Action::TRACK_CREATED:
                _parameter_manager.track_parameters(typed_event->track());
                break;

            case AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED:
                _parameter_manager.mark_processor_changed(typed_event->processor(), typed_event->time());
                break;

            case AudioGraphNotificationEvent::Action::PROCESSOR_DELETED:
                _parameter_manager.untrack_parameters(typed_event->processor());
                break;

            case AudioGraphNotificationEvent::Action::TRACK_DELETED:
                _parameter_manager.untrack_parameters(typed_event->track());
                break;

            default:
                break;
        }
    }
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

int Worker::dispatch(std::unique_ptr<Event> event)
{
    _queue.push(std::move(event));

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
            auto event = _queue.pop();

            if (event->is_engine_event())
            {
                auto typed_event = static_cast<EngineEvent*>(event.get());
                status = typed_event->execute(_engine);
            }

            if (event->is_async_work_event())
            {
                auto typed_event = static_cast<AsynchronousWorkEvent*>(event.get());
                auto response_event = typed_event->execute();
                if (response_event != nullptr)
                {
                    _dispatcher->post_event(std::move(response_event));
                }
            }

            // This is a synchronous call to the completion callback,
            // meaning it's fine that the event then goes out of scope and is deleted.
            if (event->completion_cb() != nullptr)
            {
                event->completion_cb()(event->callback_arg(), event.get(), status);
            }
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

} // end namespace sushi::internal::dispatcher
