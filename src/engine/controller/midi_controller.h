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

#ifndef SUSHI_MIDI_CONTROLLER_H
#define SUSHI_MIDI_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class MidiController : public ext::MidiController
{
public:
    MidiController(BaseEngine* engine,
                   midi_dispatcher::MidiDispatcher* midi_dispatcher);

    ~MidiController() override = default;

    int get_input_ports() const override;

    int get_output_ports() const override;

    std::vector<ext::MidiKbdConnection> get_all_kbd_input_connections() const override;

    std::vector<ext::MidiKbdConnection> get_all_kbd_output_connections() const override;

    std::vector<ext::MidiCCConnection> get_all_cc_input_connections() const override;

    std::vector<ext::MidiPCConnection> get_all_pc_input_connections() const override;

    bool get_midi_clock_output_enabled(int port) const override;

    std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
    get_cc_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::MidiPCConnection>>
    get_pc_input_connections_for_processor(int processor_id) const override;

    ext::ControlStatus set_midi_clock_output_enabled(bool enabled, int port) override;

    ext::ControlStatus
    connect_kbd_input_to_track(int track_id, ext::MidiChannel channel, int port, bool raw_midi) override;

    ext::ControlStatus
    connect_kbd_output_from_track(int track_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus connect_cc_to_parameter(int processor_id,
                                               int parameter_id,
                                               ext::MidiChannel channel,
                                               int port,
                                               int cc_number,
                                               float min_range,
                                               float max_range,
                                               bool relative_mode) override;

    ext::ControlStatus connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port, bool raw_midi) override;

    ext::ControlStatus disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number) override;

    ext::ControlStatus disconnect_pc(int processor_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_all_cc_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_pc_from_processor(int processor_id) override;

private:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_MIDI_CONTROLLER_H
