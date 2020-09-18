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

#include "midi_controller_events.h"

#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {

namespace engine {

// TODO: Change their namespace?
namespace controller_impl {

int KbdInputToTrackConnectionEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const int int_channel = ext::int_from_midi_channel(_channel);

    midi_dispatcher::MidiDispatcherStatus status;
    if(!_raw_midi)
    {
        if(_action == Action::Connect)
        {
            status = _midi_dispatcher->connect_kb_to_track(_port, _track_id, int_channel);
        }
        else
        {
            status = _midi_dispatcher->disconnect_kb_from_track(_port, // port maps to midi_input
                                                                _track_id,
                                                                int_channel);
        }
    }
    else
    {
        if(_action == Action::Connect)
        {
            status = _midi_dispatcher->connect_raw_midi_to_track(_port, _track_id, int_channel);
        }
        else
        {
            status = _midi_dispatcher->disconnect_raw_midi_from_track(_port, // port maps to midi_input
                                                                      _track_id,
                                                                      int_channel);
        }
    }

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }
    else
    {
        return EventStatus::ERROR;
    }
}

int KbdOutputToTrackConnectionEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const int int_channel = ext::int_from_midi_channel(_channel);

    midi_dispatcher::MidiDispatcherStatus status;

    if(_action == Action::Connect)
    {
        status = _midi_dispatcher->connect_track_to_output(_port, _track_id, int_channel);
    }
    else
    {
        status = _midi_dispatcher->disconnect_track_from_output(_port, _track_id, int_channel);
    }

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }
    else
    {
        return EventStatus::ERROR;
    }
}

int ConnectCCToParameterEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const int int_channel = ext::int_from_midi_channel(_channel);

    auto status = _midi_dispatcher->connect_cc_to_parameter(_port, // midi_input maps to port
                                                            _processor_id,
                                                            _parameter_name,
                                                            _cc_number,
                                                            _min_range,
                                                            _max_range,
                                                            _relative_mode,
                                                            int_channel);

    if (status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }
    else
    {
        return EventStatus::ERROR;
    }
}

int DisconnectCCEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const int int_channel = ext::int_from_midi_channel(_channel);

    const auto status = _midi_dispatcher->disconnect_cc_from_parameter(_port, // port maps to midi_input
                                                                       _processor_id,
                                                                       _cc_number,
                                                                       int_channel);

    if (status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }
    else
    {
        return EventStatus::ERROR;
    }
}

int PCToProcessorConnectionEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const int int_channel = ext::int_from_midi_channel(_channel);

    midi_dispatcher::MidiDispatcherStatus status;

    if(_action == Action::Connect)
    {
        status = _midi_dispatcher->connect_pc_to_processor(_port, // midi_input maps to port
                                                           _processor_id,
                                                           int_channel);
    }
    else
    {
        status = _midi_dispatcher->disconnect_pc_from_processor(_port,
                                                                _processor_id,
                                                                int_channel);
    }

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return EventStatus::HANDLED_OK;
    }
    else
    {
        return EventStatus::ERROR;
    }
}

int DisconnectAllCCFromProcessorEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const auto status = _midi_dispatcher->disconnect_all_cc_from_processor(_processor_id);

    if(status != midi_dispatcher::MidiDispatcherStatus::OK)
    {
        // TODO: We could do with a better error message.
        return EventStatus::ERROR;
    }

    return EventStatus::HANDLED_OK;
}

int DisconnectAllPCFromProcessorEvent::execute(engine::BaseEngine* /*engine*/) const
{
    const auto status = _midi_dispatcher->disconnect_all_pc_from_processor(_processor_id);

    if(status != midi_dispatcher::MidiDispatcherStatus::OK)
    {
        // TODO: We could do with a better error message.
        return EventStatus::ERROR;
    }

    return EventStatus::HANDLED_OK;
}


#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi
