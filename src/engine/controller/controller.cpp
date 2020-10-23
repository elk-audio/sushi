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
 * @Brief Controller object for external control of sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "controller.h"
#include "engine/base_engine.h"

#include "logging.h"
#include "control_notifications.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller")

namespace sushi {
namespace engine {

Controller::Controller(engine::BaseEngine* engine, midi_dispatcher::MidiDispatcher* midi_dispatcher) : ext::SushiControl(&_system_controller_impl,
                                                                       &_transport_controller_impl,
                                                                       &_timing_controller_impl,
                                                                       &_keyboard_controller_impl,
                                                                       &_audio_graph_controller_impl,
                                                                       &_program_controller_impl,
                                                                       &_parameter_controller_impl,
                                                                       &_midi_controller_impl,
                                                                       &_audio_routing_controller_impl,
                                                                       &_cv_gate_controller_impl,
                                                                       &_osc_controller_impl),
                                                     _engine(engine),
                                                     _system_controller_impl(engine),
                                                     _transport_controller_impl(engine),
                                                     _timing_controller_impl(engine),
                                                     _keyboard_controller_impl(engine),
                                                     _audio_graph_controller_impl(engine),
                                                     _program_controller_impl(engine),
                                                     _parameter_controller_impl(engine),
                                                     _midi_controller_impl(engine,
                                                                           midi_dispatcher,
                                                                           &_parameter_controller_impl),
                                                     _audio_routing_controller_impl(engine),
                                                     _cv_gate_controller_impl(engine),
                                                     _osc_controller_impl(engine)

{
    _event_dispatcher = engine->event_dispatcher();
    _processors = engine->processor_container();

    _event_dispatcher->subscribe_to_parameter_change_notifications(this);
    _event_dispatcher->subscribe_to_engine_notifications(this);
}

Controller::~Controller()
{
    _event_dispatcher->unsubscribe_from_parameter_change_notifications(this);
    _event_dispatcher->unsubscribe_from_engine_notifications(this);
}

ext::ControlStatus Controller::subscribe_to_notifications(ext::NotificationType type,
                                                          ext::ControlListener* listener)
{
    switch (type)
    {
        case ext::NotificationType::PARAMETER_CHANGE:
            _parameter_change_listeners.push_back(listener);
            break;
        case ext::NotificationType::PROCESSOR_ADDED:
        case ext::NotificationType::PROCESSOR_DELETED:
        case ext::NotificationType::PROCESSOR_MOVED:
            if (std::find(_processor_update_listeners.begin(), _processor_update_listeners.end(), listener) ==
                _processor_update_listeners.end()) {
                _processor_update_listeners.push_back(listener);
            }
            break;
        case ext::NotificationType::TRACK_ADDED:
        case ext::NotificationType::TRACK_DELETED:
// TODO Ilias: Add track move notification?
//      case ext::NotificationType::TRACK_CHANGED:
            if (std::find(_track_update_listeners.begin(), _track_update_listeners.end(), listener) ==
                _track_update_listeners.end()) {
                _track_update_listeners.push_back(listener);
            }
            break;
        default:
            break;
    }

    return ext::ControlStatus::OK;
}

void Controller::completion_callback(void*arg, Event*event, int status)
{
    reinterpret_cast<Controller*>(arg)->_completion_callback(event, status);
}

int Controller::process(Event* event)
{
    if (event->is_parameter_change_notification())
    {
        _notify_parameter_listeners(event);
        return EventStatus::HANDLED_OK;
    }
    else if (event->is_audio_graph_notification())
    {
        auto typed_event = static_cast<AudioGraphNotificationEvent*>(event);
        switch(typed_event->action())
        {
            case AudioGraphNotificationEvent::Action::PROCESSOR_ADDED:
            {
                _notify_processor_listeners(typed_event, ext::NotificationType::PROCESSOR_ADDED);
                break;
            }
            case AudioGraphNotificationEvent::Action::PROCESSOR_MOVED:
            {
                _notify_processor_listeners(typed_event, ext::NotificationType::PROCESSOR_MOVED);
                break;
            }
            case AudioGraphNotificationEvent::Action::PROCESSOR_DELETED:
            {
                _notify_processor_listeners(typed_event, ext::NotificationType::PROCESSOR_DELETED);
                break;
            }
            case AudioGraphNotificationEvent::Action::TRACK_ADDED:
            {
                _notify_track_listeners(typed_event, ext::NotificationType::TRACK_ADDED);
                break;
            }
// TODO Ilias: Ensure this is wired up if needed!
// case AudioGraphNotificationEvent::Action::TRACK_MOVED / CHANGED?:
            case AudioGraphNotificationEvent::Action::TRACK_DELETED:
            {
                _notify_track_listeners(typed_event, ext::NotificationType::TRACK_DELETED);
                break;
            }
        }

        return EventStatus::HANDLED_OK;
    }

    return EventStatus::NOT_HANDLED;
}

void Controller::_notify_parameter_listeners(Event* event) const
{
    auto typed_event = static_cast<ParameterChangeNotificationEvent*>(event);
    ext::ParameterChangeNotification notification((int)typed_event->processor_id(),
                                                  (int)typed_event->parameter_id(),
                                                  typed_event->float_value(),
                                                  typed_event->time());
    for (auto& listener : _parameter_change_listeners)
    {
        listener->notification(&notification);
    }
}

void Controller::_notify_track_listeners(const AudioGraphNotificationEvent* typed_event,
                                         ext::NotificationType ext_notification) const
{
    ext::TrackNotification notification(ext_notification,
                            typed_event->track(),
                            typed_event->time());
    for (auto& listener : _track_update_listeners)
    {
        listener->notification(&notification);
    }
}

void Controller::_notify_processor_listeners(const AudioGraphNotificationEvent* typed_event,
                                             ext::NotificationType ext_notification) const
{
    ext::ProcessorNotification notification(ext_notification,
                                static_cast<int>(typed_event->processor()),
                                typed_event->time());
    for (auto& listener : _processor_update_listeners)
    {
        listener->notification(&notification);
    }
}

void Controller::_completion_callback([[maybe_unused]] Event* event, int status)
{
    if (status == EventStatus::HANDLED_OK)
    {
        SUSHI_LOG_INFO("Event {}, handled OK", event->id());
    }
    else
    {
        SUSHI_LOG_WARNING("Event {} returned with error code: ", event->id(), status);
    }
}

}// namespace engine
}// namespace sushi
