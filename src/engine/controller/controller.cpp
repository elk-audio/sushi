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
#include "engine/midi_dispatcher.h"

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
                                                     _midi_controller_impl(engine, midi_dispatcher),
                                                     _audio_routing_controller_impl(engine),
                                                     _cv_gate_controller_impl(engine),
                                                     _osc_controller_impl(engine)

{
    _processors = engine->processor_container();
}

Controller::~Controller() = default;

ext::ControlStatus Controller::subscribe_to_notifications(ext::NotificationType type,
                                                          ext::ControlListener* listener)
{
    switch (type)
    {
        case ext::NotificationType::PARAMETER_CHANGE:
            _parameter_change_listeners.push_back(listener);
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
        auto typed_event = static_cast<ParameterChangeNotificationEvent*>(event);
        ext::ParameterChangeNotification notification((int)typed_event->processor_id(),
                                                      (int)typed_event->parameter_id(),
                                                      typed_event->float_value(),
                                                      typed_event->time());
        for (auto& listener : _parameter_change_listeners)
        {
            listener->notification(&notification);
        }
        return EventStatus::HANDLED_OK;
    }

    return EventStatus::NOT_HANDLED;
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
