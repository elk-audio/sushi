/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "midi_controller.h"
#include "logging.h"

#include "engine/midi_dispatcher.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

MidiController::MidiController(BaseEngine* engine, midi_dispatcher::MidiDispatcher* midi_dispatcher) : _engine(engine),
                                                                                                       _event_dispatcher(engine->event_dispatcher()),
                                                                                                       _midi_dispatcher(midi_dispatcher)
{}

int MidiController::get_input_ports() const
{
    return _midi_dispatcher->get_midi_inputs();
}

int MidiController::get_output_ports() const
{
    return _midi_dispatcher->get_midi_outputs();
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_input_connections() const
{
    return std::vector<ext::MidiKbdConnection>();
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_output_connections() const
{
    return std::vector<ext::MidiKbdConnection>();
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_input_connections() const
{
    return std::vector<ext::MidiCCConnection>();
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_output_connections() const
{
    return std::vector<ext::MidiCCConnection>();
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>> MidiController::get_cc_input_connections_for_processor(int processor_id) const
{
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, std::vector<ext::MidiCCConnection>()};
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>> MidiController::get_cc_output_connections_for_processor(int processor_id) const
{
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, std::vector<ext::MidiCCConnection>()};
}

ext::ControlStatus MidiController::connect_kbd_input_to_track(int track_id,
                                                              ext::MidiChannel channel,
                                                              int port,
                                                              bool raw_midi)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_kbd_output_from_track(int track_id,
                                                                 ext::MidiChannel channel,
                                                                 int port,
                                                                 bool raw_midi)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_cc_to_parameter(int processor_id,
                                                           ext::MidiChannel channel,
                                                           int port,
                                                           int cc_number,
                                                           int min_range,
                                                           int max_range,
                                                           bool relative_mode)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_pc(int processor_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_all_cc_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_all_pc_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi
