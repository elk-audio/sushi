/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_MIDI_CONTROLLER_H
#define SUSHI_MIDI_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"

namespace sushi::internal::engine::controller_impl {

class MidiController : public control::MidiController
{
public:
    MidiController(BaseEngine* engine,
                   midi_dispatcher::MidiDispatcher* midi_dispatcher);

    ~MidiController() override = default;

    int get_input_ports() const override;

    int get_output_ports() const override;

    std::vector<control::MidiKbdConnection> get_all_kbd_input_connections() const override;

    std::vector<control::MidiKbdConnection> get_all_kbd_output_connections() const override;

    std::vector<control::MidiCCConnection> get_all_cc_input_connections() const override;

    std::vector<control::MidiPCConnection> get_all_pc_input_connections() const override;

    bool get_midi_clock_output_enabled(int port) const override;

    std::pair<control::ControlStatus, std::vector<control::MidiCCConnection>>
    get_cc_input_connections_for_processor(int processor_id) const override;

    std::pair<control::ControlStatus, std::vector<control::MidiPCConnection>>
    get_pc_input_connections_for_processor(int processor_id) const override;

    control::ControlStatus set_midi_clock_output_enabled(bool enabled, int port) override;

    control::ControlStatus connect_kbd_input_to_track(int track_id,
                                                      control::MidiChannel channel,
                                                      int port,
                                                      bool raw_midi) override;

    control::ControlStatus connect_kbd_output_from_track(int track_id,
                                                         control::MidiChannel channel,
                                                         int port) override;

    control::ControlStatus connect_cc_to_parameter(int processor_id,
                                                   int parameter_id,
                                                   control::MidiChannel channel,
                                                   int port,
                                                   int cc_number,
                                                   float min_range,
                                                   float max_range,
                                                   bool relative_mode) override;

    control::ControlStatus connect_pc_to_processor(int processor_id,
                                                   control::MidiChannel channel,
                                                   int port) override;

    control::ControlStatus disconnect_kbd_input(int track_id,
                                                control::MidiChannel channel,
                                                int port, bool raw_midi) override;

    control::ControlStatus disconnect_kbd_output(int track_id,
                                                 control::MidiChannel channel,
                                                 int port) override;

    control::ControlStatus disconnect_cc(int processor_id,
                                         control::MidiChannel channel,
                                         int port,
                                         int cc_number) override;

    control::ControlStatus disconnect_pc(int processor_id, control::MidiChannel channel, int port) override;

    control::ControlStatus disconnect_all_cc_from_processor(int processor_id) override;

    control::ControlStatus disconnect_all_pc_from_processor(int processor_id) override;

private:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_MIDI_CONTROLLER_H
