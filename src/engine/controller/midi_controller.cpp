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
 * @brief Implementation of external control interface for sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"

#include "midi_controller.h"

#include "controller_common.h"

namespace sushi::internal::engine::controller_impl {

    control::MidiCCConnection populate_cc_connection(const midi_dispatcher::CCInputConnection& connection)
{
    control::MidiCCConnection ext_connection;

    ext_connection.processor_id = connection.input_connection.target;
    ext_connection.parameter_id = connection.input_connection.parameter;
    ext_connection.min_range = connection.input_connection.min_range;
    ext_connection.max_range = connection.input_connection.max_range;
    ext_connection.relative_mode = connection.input_connection.relative;
    ext_connection.channel = to_external_midi_channel(connection.channel);
    ext_connection.port = connection.port;
    ext_connection.cc_number = connection.cc;

    return ext_connection;
}

control::MidiPCConnection populate_pc_connection(const midi_dispatcher::PCInputConnection& connection)
{
    control::MidiPCConnection ext_connection;

    ext_connection.processor_id = connection.processor_id;
    ext_connection.channel = to_external_midi_channel(connection.channel);
    ext_connection.port = connection.port;

    return ext_connection;
}


MidiController::MidiController(BaseEngine* engine,
                               midi_dispatcher::MidiDispatcher* midi_dispatcher) : _event_dispatcher(engine->event_dispatcher()),
                                                                                   _midi_dispatcher(midi_dispatcher) {}

int MidiController::get_input_ports() const
{
    return _midi_dispatcher->get_midi_inputs();
}

int MidiController::get_output_ports() const
{
    return _midi_dispatcher->get_midi_outputs();
}

std::vector<control::MidiKbdConnection> MidiController::get_all_kbd_input_connections() const
{
    std::vector<control::MidiKbdConnection> returns;

    const auto connections = _midi_dispatcher->get_all_kb_input_connections();
    for (auto connection : connections)
    {
        control::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection.input_connection.target;
        ext_connection.port = connection.port;
        ext_connection.channel = to_external_midi_channel(connection.channel);
        ext_connection.raw_midi = connection.raw_midi;
        returns.push_back(ext_connection);
    }

    return returns;
}

std::vector<control::MidiKbdConnection> MidiController::get_all_kbd_output_connections() const
{
    std::vector<control::MidiKbdConnection> returns;

    const auto connections = _midi_dispatcher->get_all_kb_output_connections();
    for (auto connection : connections)
    {
        control::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection.track_id;
        ext_connection.port = connection.port;
        ext_connection.channel = to_external_midi_channel(connection.channel);
        ext_connection.raw_midi = false;
        returns.push_back(ext_connection);
    }

    return returns;
}

std::vector<control::MidiCCConnection> MidiController::get_all_cc_input_connections() const
{
    std::vector<control::MidiCCConnection> returns;

    const auto connections = _midi_dispatcher->get_all_cc_input_connections();
    for (auto connection : connections)
    {
        auto ext_connection = populate_cc_connection(connection);
        returns.push_back(ext_connection);
    }

    return returns;
}

std::vector<control::MidiPCConnection> MidiController::get_all_pc_input_connections() const
{
    std::vector<control::MidiPCConnection> returns;

    const auto connections = _midi_dispatcher->get_all_pc_input_connections();
    for (auto connection : connections)
    {
        auto ext_connection = populate_pc_connection(connection);
        returns.push_back(ext_connection);
    }

    return returns;
}


bool MidiController::get_midi_clock_output_enabled(int port) const
{
    return _midi_dispatcher->midi_clock_enabled(port);
}

control::ControlStatus MidiController::set_midi_clock_output_enabled(bool enabled, int port)
{
    auto lambda = [=] () -> int
    {
        auto status = _midi_dispatcher->enable_midi_clock(enabled, port);
        return status == midi_dispatcher::MidiDispatcherStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

std::pair<control::ControlStatus, std::vector<control::MidiCCConnection>>
MidiController::get_cc_input_connections_for_processor(int processor_id) const
{
    std::pair<control::ControlStatus, std::vector<control::MidiCCConnection>> returns;
    returns.first = control::ControlStatus::OK;

    const auto connections = _midi_dispatcher->get_cc_input_connections_for_processor(processor_id);
    for (auto connection : connections)
    {
        auto ext_connection = populate_cc_connection(connection);
        returns.second.push_back(ext_connection);
    }

    return returns;
}

std::pair<control::ControlStatus, std::vector<control::MidiPCConnection>>
MidiController::get_pc_input_connections_for_processor(int processor_id) const
{
    std::pair<control::ControlStatus, std::vector<control::MidiPCConnection>> returns;
    returns.first = control::ControlStatus::OK;

    const auto connections = _midi_dispatcher->get_pc_input_connections_for_processor(processor_id);
    for (auto connection : connections)
    {
        auto ext_connection = populate_pc_connection(connection);
        returns.second.push_back(ext_connection);
    }

    return returns;
}

control::ControlStatus MidiController::connect_kbd_input_to_track(int track_id,
                                                                  control::MidiChannel channel,
                                                                  int port,
                                                                  bool raw_midi)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;
        if (!raw_midi)
        {
            status = _midi_dispatcher->connect_kb_to_track(port, track_id, int_channel);
        }
        else
        {
            status = _midi_dispatcher->connect_raw_midi_to_track(port, track_id, int_channel);
        }

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::connect_kbd_output_from_track(int track_id,
                                                                     control::MidiChannel channel,
                                                                     int port)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;
        status = _midi_dispatcher->connect_track_to_output(port, track_id, int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::connect_cc_to_parameter(int processor_id,
                                                               int parameter_id,
                                                               control::MidiChannel channel,
                                                               int port,
                                                               int cc_number,
                                                               float min_range,
                                                               float max_range,
                                                               bool relative_mode)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        auto status = _midi_dispatcher->connect_cc_to_parameter(port, // midi_input maps to port
                                                                processor_id,
                                                                parameter_id,
                                                                cc_number,
                                                                min_range,
                                                                max_range,
                                                                relative_mode,
                                                                int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::connect_pc_to_processor(int processor_id,
                                                               control::MidiChannel channel, int port)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;

        status = _midi_dispatcher->connect_pc_to_processor(port, // midi_input maps to port
                                                           processor_id,
                                                           int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));
    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_kbd_input(int track_id,
                                                            control::MidiChannel channel, int port, bool raw_midi)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=]() -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;
        if (!raw_midi)
        {
            status = _midi_dispatcher->disconnect_kb_from_track(port, // port maps to midi_input
                                                                track_id,
                                                                int_channel);
        }
        else
        {
            status = _midi_dispatcher->disconnect_raw_midi_from_track(port, // port maps to midi_input
                                                                      track_id,
                                                                      int_channel);
        }

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_kbd_output(int track_id, control::MidiChannel channel, int port)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;
        status = _midi_dispatcher->disconnect_track_from_output(port, track_id, int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_cc(int processor_id,
                                                     control::MidiChannel channel, int port, int cc_number)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        const auto status = _midi_dispatcher->disconnect_cc_from_parameter(port, // port maps to midi_input
                                                                           processor_id,
                                                                           cc_number,
                                                                           int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_pc(int processor_id, control::MidiChannel channel, int port)
{
    const int int_channel = int_from_ext_midi_channel(channel);

    auto lambda = [=] () -> int
    {
        midi_dispatcher::MidiDispatcherStatus status;

        status = _midi_dispatcher->disconnect_pc_from_processor(port,
                                                                processor_id,
                                                                int_channel);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_all_cc_from_processor(int processor_id)
{
    auto lambda = [=] () -> int
    {
        const auto status = _midi_dispatcher->disconnect_all_cc_from_processor(processor_id);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

control::ControlStatus MidiController::disconnect_all_pc_from_processor(int processor_id)
{
    auto lambda = [=] () -> int
    {
        const auto status = _midi_dispatcher->disconnect_all_pc_from_processor(processor_id);

        if (status == midi_dispatcher::MidiDispatcherStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }
        else
        {
            return EventStatus::ERROR;
        }
    };

    std::unique_ptr<Event> e;
    e.reset(new LambdaEvent(lambda, IMMEDIATE_PROCESS));
    _event_dispatcher->post_event(std::move(e));

    return control::ControlStatus::OK;
}

} // end namespace sushi::internal::engine::controller_impl
