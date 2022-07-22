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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <control_notifications.h>
#include "control_interface.h"
#include "engine/base_event_dispatcher.h"
#include "engine/transport.h"
#include "library/base_performance_timer.h"
#include "engine/base_processor_container.h"
#include "library/event_interface.h"

#include "system_controller.h"
#include "transport_controller.h"
#include "timing_controller.h"
#include "keyboard_controller.h"
#include "audio_graph_controller.h"
#include "parameter_controller.h"
#include "program_controller.h"
#include "midi_controller.h"
#include "audio_routing_controller.h"
#include "cv_gate_controller.h"
#include "osc_controller.h"
#include "session_controller.h"

#ifndef SUSHI_CONTROLLER_H
#define SUSHI_CONTROLLER_H

namespace sushi {

namespace midi_dispatcher {
class MidiDispatcher;
}

namespace control_frontend {
class OSCFrontend;
}

namespace audio_frontend {
class BaseAudioFrontend;
}

namespace engine {

class BaseEngine;

class Controller : public ext::SushiControl, EventPoster
{
public:
    Controller(engine::BaseEngine* engine,
               midi_dispatcher::MidiDispatcher* midi_dispatcher,
               audio_frontend::BaseAudioFrontend* audio_frontend);

    ~Controller() override;

    ext::ControlStatus subscribe_to_notifications(ext::NotificationType type, ext::ControlListener* listener) override;

    /* Inherited from EventPoster */
    int process(Event* event) override;

    int poster_id() override { return EventPosterId::CONTROLLER; }

    static void completion_callback(void* arg, Event* event, int status);

    void set_osc_frontend(control_frontend::OSCFrontend* osc_frontend);

private:

    void _completion_callback(Event* event, int status);

    void _handle_engine_notifications(const EngineNotificationEvent* event);

    void _handle_audio_graph_notifications(const AudioGraphNotificationEvent* event);

    void _notify_processor_listeners(const AudioGraphNotificationEvent* typed_event,
                                     ext::ProcessorAction action) const;

    void _notify_track_listeners(const AudioGraphNotificationEvent* typed_event,
                                 ext::TrackAction action) const;

    void _notify_transport_listeners(const ext::TransportNotification& notification) const;

    void _notify_parameter_listeners(Event* event) const;

    void _notify_property_listeners(Event* event) const;

    void _notify_timing_listeners(const EngineTimingNotificationEvent* event) const;

    std::vector<ext::ControlListener*>      _parameter_change_listeners;
    std::vector<ext::ControlListener*>      _property_change_listeners;
    std::vector<ext::ControlListener*>      _processor_update_listeners;
    std::vector<ext::ControlListener*>      _track_update_listeners;
    std::vector<ext::ControlListener*>      _transport_update_listeners;
    std::vector<ext::ControlListener*>      _cpu_timing_update_listeners;

    const engine::BaseProcessorContainer*   _processors;

    controller_impl::SystemController       _system_controller_impl;
    controller_impl::TransportController    _transport_controller_impl;
    controller_impl::TimingController       _timing_controller_impl;
    controller_impl::KeyboardController     _keyboard_controller_impl;
    controller_impl::AudioGraphController   _audio_graph_controller_impl;
    controller_impl::ProgramController      _program_controller_impl;
    controller_impl::ParameterController    _parameter_controller_impl;
    controller_impl::MidiController         _midi_controller_impl;
    controller_impl::AudioRoutingController _audio_routing_controller_impl;
    controller_impl::CvGateController       _cv_gate_controller_impl;
    controller_impl::OscController          _osc_controller_impl;
    controller_impl::SessionController      _session_controller_impl;

    dispatcher::BaseEventDispatcher*        _event_dispatcher;
};

} //namespace engine
} //namespace sushi
#endif //SUSHI_CONTROLLER_H
