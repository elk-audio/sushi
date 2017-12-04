
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
    switch (event->type())
    {
        case EventType::KEYBOARD_EVENT:
            return _process_kb_event(static_cast<KeyboardEvent*>(event));

        case EventType::PARAMETER_CHANGE:
        case EventType::STRING_PROPERTY_CHANGE:
            return _process_parameter_change_event(static_cast<ParameterChangeEvent*>(event));

        case EventType::ADD_TRACK:
            return _process_add_track_event(static_cast<AddTrackEvent*>(event));

        case EventType::REMOVE_TRACK:
            return _process_remove_track_event(static_cast<RemoveTrackEvent*>(event));

        case EventType::ADD_PROCESSOR:
            return _process_add_processor_event(static_cast<AddProcessorEvent*>(event));

        case EventType::REMOVE_PROCESSOR:
            return _process_remove_processor_event(static_cast<RemoveProcessorEvent*>(event));


        //case EventType::ASYNCHRONOUS_WORK:
        //    return _process_async_work_event(static_cast<AsynchronousWorkEvent*>(event));

        default:
            return EventStatus::UNRECOGNIZED_TYPE;
    }
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

int EventDispatcher::_process_kb_event(KeyboardEvent* event)
{
    ObjectId processor_id;
    if (event->access_by_id())
    {
        processor_id = event->processor_id();
    }
    else
    {
        auto processor_node = _engine->processor_id_from_name(event->processor());
        if (processor_node.first != engine::EngineReturnStatus::OK)
        {
            return EventStatus::NOT_HANDLED;
        }
        processor_id = processor_node.second;
    }
    // TODO - handle translation from real time to sample offset.
    int offset = 0;
    RtEvent rt_event;
    switch (event->subtype())
    {
        case KeyboardEvent::Subtype::NOTE_ON:
            rt_event = RtEvent::make_note_on_event(processor_id, offset, event->note(), event->velocity());
            break;

        case KeyboardEvent::Subtype::NOTE_OFF:
            rt_event = RtEvent::make_note_off_event(processor_id, offset, event->note(), event->velocity());
            break;

        case KeyboardEvent::Subtype::NOTE_AFTERTOUCH:
            rt_event = RtEvent::make_note_aftertouch_event(processor_id, offset, event->note(), event->velocity());
            break;

        case KeyboardEvent::Subtype::PITCH_BEND:
            // TODO - implement pb event
            //rt_event = RtEvent::make_p
            return EventStatus::NOT_HANDLED;

        case KeyboardEvent::Subtype::POLY_AFTERTOUCH:
            // TODO - implement poly aftertouch event
            //rt_event = RtEvent::make_pol
            return EventStatus::NOT_HANDLED;

        case KeyboardEvent::Subtype::WRAPPED_MIDI:
            rt_event = RtEvent::make_wrapped_midi_event(processor_id, offset, event->midi_data());
            break;
    }
    // TODO - handle case when queue is full.
    _out_rt_queue->push(rt_event);
    return EventStatus::HANDLED_OK;
}

int EventDispatcher::_process_parameter_change_event(ParameterChangeEvent* event)
{
    ObjectId processor_id;
    ObjectId parameter_id;
    if (event->access_by_id())
    {
        processor_id = event->processor_id();
        parameter_id = event->parameter_id();
    }
    else
    {
        auto processor_node = _engine->processor_id_from_name(event->processor());
        if (processor_node.first != engine::EngineReturnStatus::OK)
        {
            return EventStatus::NOT_HANDLED;
        }
        auto parameter_node = _engine->parameter_id_from_name(event->processor(), event->parameter());
        if (parameter_node.first != engine::EngineReturnStatus::OK)
        {
            return EventStatus::NOT_HANDLED;
        }
        processor_id = processor_node.second;
        parameter_id = parameter_node.second;
    }
    // TODO - handle translation from real time to sample offset.
    int offset = 0;
    RtEvent rt_event;
    switch (event->subtype())
    {
        case ParameterChangeEvent::Subtype::INT_PARAMETER_CHANGE:
            rt_event = RtEvent::make_parameter_change_event(processor_id, offset, parameter_id, event->int_value());
            break;

        case ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE:
            rt_event = RtEvent::make_parameter_change_event(processor_id, offset, parameter_id, event->float_value());
            break;

        case ParameterChangeEvent::Subtype::BOOL_PARAMETER_CHANGE:
            rt_event = RtEvent::make_parameter_change_event(processor_id, offset, parameter_id, event->bool_value());
            break;

        case ParameterChangeEvent::Subtype::BLOB_PROPERTY_CHANGE:
            // TODO - implement blob parameter change event
            return EventStatus::NOT_HANDLED;

        case ParameterChangeEvent::Subtype::STRING_PROPERTY_CHANGE:
            auto typed_event = static_cast<StringPropertyChangeEvent*>(event);
            auto string_value = new std::string(typed_event->string_value());
            rt_event = RtEvent::make_string_parameter_change_event(processor_id, offset, parameter_id, string_value);
            break;
    }
    _out_rt_queue->push(rt_event);
    return EventStatus::HANDLED_OK;
}

int EventDispatcher::_process_async_work_event(AsynchronousWorkEvent* event)
{
    return EventStatus::NOT_HANDLED;
}

int EventDispatcher::_process_rt_event(RtEvent &event)
{
    MIND_LOG_INFO("Dispatcher: processing rt event");
    switch (event.type())
    {
        case RtEventType::ASYNC_WORK:
        {
            auto typed_event = event.async_work_event();
            MIND_LOG_INFO("Got an ASYNC_WORK event with id {}", typed_event->event_id());
            int status = typed_event->callback()(typed_event->callback_data(), typed_event->event_id());
            _out_rt_queue->push(RtEvent::make_async_work_completion_event(typed_event->processor_id(),
                                                                         typed_event->event_id(),
                                                                         status));
            break;
        }
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
            _process_rt_keyboard_events(event.keyboard_event());
            break;

        case RtEventType::BOOL_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::FLOAT_PARAMETER_CHANGE:
            _process_rt_parameter_change_events(event.parameter_change_event());
            break;

        default:
            return EventStatus::UNRECOGNIZED_TYPE;
    }
    return EventStatus::HANDLED_OK;
}

int EventDispatcher::_process_rt_keyboard_events(const KeyboardRtEvent* event)
{
    // TODO - map offset to real world timestamp
    Time timestamp = PROCESS_NOW;
    /*auto processor = _engine->processor_name_from_id(event->processor_id());
    if (processor.first != engine::EngineReturnStatus::OK)
    {
        return 0;
    }*/
    KeyboardEvent::Subtype subtype;
    switch (event->type())
    {
        case RtEventType::NOTE_ON:
            subtype = KeyboardEvent::Subtype::NOTE_ON;
            break;
        case RtEventType::NOTE_OFF:
            subtype = KeyboardEvent::Subtype::NOTE_OFF;
            break;
        case RtEventType::NOTE_AFTERTOUCH:
            subtype = KeyboardEvent::Subtype::NOTE_AFTERTOUCH;
            break;
        default:
            subtype = KeyboardEvent::Subtype::WRAPPED_MIDI;
            // TODO - fill list
    }
    KeyboardEvent e(subtype, event->processor_id(), event->note(), event->velocity(), timestamp);
    _publish_keyboard_events(&e);
    return EventStatus::HANDLED_OK;
}

int EventDispatcher::_process_rt_parameter_change_events(const ParameterChangeRtEvent* event)
{
    Time timestamp = PROCESS_NOW;
    engine::EngineReturnStatus status;
    std::string processor_name;
    std::tie(status, processor_name) = _engine->processor_name_from_id(event->processor_id());
    if (status == engine::EngineReturnStatus::OK)
    {
        std::string parameter_name;
        std::tie(status, parameter_name) = _engine->parameter_name_from_id(processor_name,
                                                                           event->param_id());
        if (status == engine::EngineReturnStatus::OK)
        {
            ParameterChangeNotificationEvent::Subtype subtype;
            switch (event->type())
            {
                case RtEventType::BOOL_PARAMETER_CHANGE:
                    subtype = ParameterChangeNotificationEvent::Subtype::BOOL_PARAMETER_CHANGE_NOT;
                    break;
                case RtEventType::INT_PARAMETER_CHANGE:
                    subtype = ParameterChangeNotificationEvent::Subtype::INT_PARAMETER_CHANGE_NOT;
                    break;
                case RtEventType::FLOAT_PARAMETER_CHANGE:
                    subtype = ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT;
                    break;
                default:
                    return 0;
            }
            ParameterChangeNotificationEvent e(subtype, processor_name, parameter_name, event->value(), timestamp);
            _publish_parameter_events(&e);
        }
    }
    return 0;
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

int EventDispatcher::_process_add_track_event(AddTrackEvent* event)
{
    auto status = _engine->create_track(event->name(), event->channels());
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN_NAME:
        default:
            return AddTrackEvent::Status::INVALID_NAME;
    }
}

int EventDispatcher::_process_remove_track_event(RemoveTrackEvent* event)
{
    auto status = _engine->delete_track(event->name());
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN_NAME:
        default:
            return RemoveTrackEvent::Status::INVALID_TRACK;
    }
}

int EventDispatcher::_process_add_processor_event(AddProcessorEvent* event)
{
    engine::PluginType plugin_type;
    switch (event->processor_type())
    {
        case AddProcessorEvent::ProcessorType::INTERNAL:
            plugin_type = engine::PluginType::INTERNAL;
            break;

        case AddProcessorEvent::ProcessorType::VST2X:
            plugin_type = engine::PluginType::VST2X;
            break;

        case AddProcessorEvent::ProcessorType::VST3X:
            plugin_type = engine::PluginType::VST3X;
            break;
    }

    // TODO where to do validation?
    auto status = _engine->add_plugin_to_track(event->track(), event->uid(), event->name(), event->file(), plugin_type);
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN_NAME:
            return AddProcessorEvent::Status::INVALID_NAME;

        case engine::EngineReturnStatus::INVALID_TRACK:
            return AddProcessorEvent::Status::INVALID_CHAIN;

        case engine::EngineReturnStatus::INVALID_PLUGIN_UID:
            return AddProcessorEvent::Status::INVALID_UID;

        default:
            return AddProcessorEvent::Status::INVALID_PLUGIN;
    }
}

int EventDispatcher::_process_remove_processor_event(RemoveProcessorEvent* event)
{
    auto status = _engine->remove_plugin_from_track(event->track(), event->name());
    switch (status)
    {
        case engine::EngineReturnStatus::OK:
            return EventStatus::HANDLED_OK;

        case engine::EngineReturnStatus::INVALID_PLUGIN_NAME:
            return RemoveProcessorEvent::Status::INVALID_NAME;

        case engine::EngineReturnStatus::INVALID_TRACK:
        default:
            return RemoveProcessorEvent::Status::INVALID_CHAIN;
    }
}


} // end namespace dispatcher
} // end namespace sushi