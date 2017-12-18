
#include "event_dispatcher.h"
#include "engine/engine.h"
#include "logging.h"

namespace sushi {
namespace dispatcher {

MIND_GET_LOGGER;

EventDispatcher::EventDispatcher(engine::BaseEngine* engine,
                                 RtEventFifo* in_rt_queue,
                                 RtEventFifo* out_rt_queue) : _engine{engine},
                                                              _in_rt_queue{in_rt_queue},
                                                              _out_rt_queue{out_rt_queue}
{
    for (auto& p : _posters)
    {
        p = nullptr;
    }
    _posters[EventPosterId::AUDIO_ENGINE] = this;
}

void EventDispatcher::post_event(Event* event)
{
    std::lock_guard<std::mutex> lock(_in_queue_mutex);
    _in_queue.push_front(event);
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
    _running = true;
    _event_thread = std::thread(&EventDispatcher::_event_loop, this);
}

void EventDispatcher::stop()
{
    _running = false;
    if (_event_thread.joinable())
    {
        _event_thread.join();
    }
}

EventDispatcherStatus EventDispatcher::subscribe_to_keyboard_events(EventPoster* receiver)
{
    for (auto r : _keyboard_event_listeners)
    {
        if (r == receiver) return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _keyboard_event_listeners.push_back(receiver);
    return EventDispatcherStatus::OK;
}

EventDispatcherStatus EventDispatcher::subscribe_to_parameter_change_notifications(EventPoster* receiver)
{
    for (auto r : _parameter_change_listeners)
    {
        if (r == receiver) return EventDispatcherStatus::ALREADY_SUBSCRIBED;
    }
    _parameter_change_listeners.push_back(receiver);
    return EventDispatcherStatus::OK;
}

int EventDispatcher::process(Event* event)
{
    /*if (event->process_asynchronously())
    {
        // TODO - send to low prio handler when it's implemented
        return EventStatus::NOT_HANDLED;
    }*/
    if (event->maps_to_rt_event())
    {
        // TODO - handle translation from real time to sample offset.
        int sample_offset = 0;
        // TODO - handle case when queue is full.
        _out_rt_queue->push(event->to_rt_event(sample_offset));
        return EventStatus::HANDLED_OK;
    }
    if (event->is_engine_event())
    {
        auto typed_event = static_cast<EngineEvent*>(event);
        return typed_event->execute(_engine);
    }
    return EventStatus::UNRECOGNIZED_EVENT;
}

void EventDispatcher::_event_loop()
{
    do
    {
        auto start_time = std::chrono::system_clock::now();
        /* Handle incoming Events */
        while (!_in_queue.empty())
        {
            Event* event;
            {
                std::lock_guard<std::mutex> lock(_in_queue_mutex);
                event = _in_queue.back();
                _in_queue.pop_back();
            }
            assert(event->receiver() < static_cast<int>(_posters.size()));
            EventPoster* receiver = _posters[event->receiver()];
            int status;
            if (receiver != nullptr)
            {
                status = _posters[event->receiver()]->process(event);
            }
            else
            {
                status = EventStatus::UNRECOGNIZED_RECEIVER;
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
    // TODO - handle translation from real time to sample offset.
    Time timestamp = PROCESS_NOW;
    Event* event = Event::from_rt_event(rt_event, timestamp);
    if (event != nullptr)
    {
        if (event->is_keyboard_event())
        {
            _publish_keyboard_events(event);
        }
        if (event->is_parameter_change_event())
        {
            _publish_parameter_events(event);
        }
        // TODO - better lifetime management of events
        delete event;
        return EventStatus::HANDLED_OK;
    }
    switch (rt_event.type())
    {
        case RtEventType::ASYNC_WORK:
        {
            auto typed_event = rt_event.async_work_event();
            MIND_LOG_INFO("Got an ASYNC_WORK event with id {}", typed_event->event_id());
            int status = typed_event->callback()(typed_event->callback_data(), typed_event->event_id());
            _out_rt_queue->push(RtEvent::make_async_work_completion_event(typed_event->processor_id(),
                                                                         typed_event->event_id(),
                                                                         status));
            break;
        }
        default:
            return EventStatus::UNRECOGNIZED_EVENT;
    }
    return EventStatus::HANDLED_OK;
}


void EventDispatcher::_publish_keyboard_events(Event* event)
{
    for (auto& listener : _keyboard_event_listeners)
    {
        listener->process(event);
    }

}

void EventDispatcher::_publish_parameter_events(Event* event)
{
    for (auto& listener : _parameter_change_listeners)
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
    for (auto i = _parameter_change_listeners.begin(); i != _parameter_change_listeners.end(); ++i)
    {
        if (*i == receiver)
        {
            _parameter_change_listeners.erase(i);
            return EventDispatcherStatus::OK;
        }
    }
    return EventDispatcherStatus::UNKNOWN_POSTER;}


} // end namespace dispatcher
} // end namespace sushi