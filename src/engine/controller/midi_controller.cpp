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

#include "engine/midi_dispatcher.h"

#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

inline ext::MidiChannel midi_channel_from_int(int channel_int)
{
    switch (channel_int)
    {
        case 0:  return sushi::ext::MidiChannel::MIDI_CH_1;
        case 1:  return sushi::ext::MidiChannel::MIDI_CH_2;
        case 2:  return sushi::ext::MidiChannel::MIDI_CH_3;
        case 3:  return sushi::ext::MidiChannel::MIDI_CH_4;
        case 4:  return sushi::ext::MidiChannel::MIDI_CH_5;
        case 5:  return sushi::ext::MidiChannel::MIDI_CH_6;
        case 6:  return sushi::ext::MidiChannel::MIDI_CH_7;
        case 7:  return sushi::ext::MidiChannel::MIDI_CH_8;
        case 8:  return sushi::ext::MidiChannel::MIDI_CH_9;
        case 9:  return sushi::ext::MidiChannel::MIDI_CH_10;
        case 10: return sushi::ext::MidiChannel::MIDI_CH_11;
        case 11: return sushi::ext::MidiChannel::MIDI_CH_12;
        case 12: return sushi::ext::MidiChannel::MIDI_CH_13;
        case 13: return sushi::ext::MidiChannel::MIDI_CH_14;
        case 14: return sushi::ext::MidiChannel::MIDI_CH_15;
        case 15: return sushi::ext::MidiChannel::MIDI_CH_16;
        case 16: return sushi::ext::MidiChannel::MIDI_CH_OMNI;
        default: return sushi::ext::MidiChannel::MIDI_CH_OMNI;
    }
}

inline int int_from_midi_channel(ext::MidiChannel channel)
{
    switch (channel)
    {
        case sushi::ext::MidiChannel::MIDI_CH_1: return 0;
        case sushi::ext::MidiChannel::MIDI_CH_2: return 1;
        case sushi::ext::MidiChannel::MIDI_CH_3: return 2;
        case sushi::ext::MidiChannel::MIDI_CH_4: return 3;
        case sushi::ext::MidiChannel::MIDI_CH_5: return 4;
        case sushi::ext::MidiChannel::MIDI_CH_6: return 5;
        case sushi::ext::MidiChannel::MIDI_CH_7: return 6;
        case sushi::ext::MidiChannel::MIDI_CH_8: return 7;
        case sushi::ext::MidiChannel::MIDI_CH_9: return 8;
        case sushi::ext::MidiChannel::MIDI_CH_10: return 9;
        case sushi::ext::MidiChannel::MIDI_CH_11: return 10;
        case sushi::ext::MidiChannel::MIDI_CH_12: return 11;
        case sushi::ext::MidiChannel::MIDI_CH_13: return 12;
        case sushi::ext::MidiChannel::MIDI_CH_14: return 13;
        case sushi::ext::MidiChannel::MIDI_CH_15: return 14;
        case sushi::ext::MidiChannel::MIDI_CH_16: return 15;
        case sushi::ext::MidiChannel::MIDI_CH_OMNI: return 16;
    }

    return -1;
}

inline ext::MidiCCConnection populate_cc_connection(const midi_dispatcher::CC_InputConnection& connection)
{
    ext::MidiCCConnection ext_connection;

    ext_connection.processor_id = connection.input_connection.target;
    ext_connection.parameter_id = connection.input_connection.parameter;
    ext_connection.min_range = connection.input_connection.min_range;
    ext_connection.max_range = connection.input_connection.max_range;
    ext_connection.relative_mode = connection.input_connection.relative;
    ext_connection.channel = midi_channel_from_int(connection.channel);
    ext_connection.port = connection.port;
    ext_connection.cc_number = connection.cc;

    return ext_connection;
}

inline ext::MidiPCConnection populate_pc_connection(const midi_dispatcher::PC_InputConnection& connection)
{
    ext::MidiPCConnection ext_connection;

    ext_connection.processor_id = connection.processor_id;
    ext_connection.channel = midi_channel_from_int(connection.channel);
    ext_connection.port = connection.port;

    return ext_connection;
}

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

MidiController::MidiController(BaseEngine* engine,
                               midi_dispatcher::MidiDispatcher* midi_dispatcher,
                               ext::AudioGraphController* audio_graph_controller,
                               ext::ParameterController* parameter_controller) : _engine(engine),
                                                                                 _event_dispatcher(engine->event_dispatcher()),
                                                                                 _midi_dispatcher(midi_dispatcher),
                                                                                 _graph_controller(audio_graph_controller),
                                                                                 _parameter_controller(parameter_controller)
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
    std::vector<ext::MidiKbdConnection> returns;

    auto connections = _midi_dispatcher->get_all_kb_input_connections();

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        ext::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection->input_connection.target;
        ext_connection.port = connection->port;
        ext_connection.channel = midi_channel_from_int(connection->channel);
        ext_connection.raw_midi = connection->raw_midi;
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_output_connections() const
{
    std::vector<ext::MidiKbdConnection> returns;

    auto connections = _midi_dispatcher->get_all_kb_output_connections();

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        ext::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection->track_id;
        ext_connection.port = connection->port;
        ext_connection.channel = midi_channel_from_int(connection->channel);
        ext_connection.raw_midi = false;
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_input_connections() const
{
    std::vector<ext::MidiCCConnection> returns;

    auto connections = _midi_dispatcher->get_all_cc_input_connections();

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = populate_cc_connection(*connection);
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiPCConnection> MidiController::get_all_pc_input_connections() const
{
    std::vector<ext::MidiPCConnection> returns;

    auto connections = _midi_dispatcher->get_all_pc_input_connections();

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = populate_pc_connection(*connection);
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
MidiController::get_cc_input_connections_for_processor(int processor_id) const
{
    std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>> returns;

    const auto connections = _midi_dispatcher->get_cc_input_connections_for_processor(processor_id);

    returns.first = ext::ControlStatus::OK;

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = populate_cc_connection(*connection);
        returns.second.emplace_back(ext_connection);
    }

    return returns;
}

std::pair<ext::ControlStatus, std::vector<ext::MidiPCConnection>>
MidiController::get_pc_input_connections_for_processor(int processor_id) const
{
    std::pair<ext::ControlStatus, std::vector<ext::MidiPCConnection>> returns;

    const auto connections = _midi_dispatcher->get_pc_input_connections_for_processor(processor_id);

    returns.first = ext::ControlStatus::OK;

    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = populate_pc_connection(*connection);
        returns.second.emplace_back(ext_connection);
    }

    return returns;
}

ext::ControlStatus MidiController::connect_kbd_input_to_track(int track_id,
                                                              ext::MidiChannel channel,
                                                              int port,
                                                              bool raw_midi)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
        return track_info_tuple.first;

    const auto track_info = track_info_tuple.second;
    const std::string track_name = track_info.name;

    midi_dispatcher::MidiDispatcherStatus status;
    if(!raw_midi)
    {
        status = _midi_dispatcher->connect_kb_to_track(midi_input, track_name, int_channel);
    }
    else
    {
        status = _midi_dispatcher->connect_raw_midi_to_track(midi_input, track_name, int_channel);
    }

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::connect_kbd_output_from_track(int track_id,
                                                                 ext::MidiChannel channel,
                                                                 int port)
{
    const int int_channel = int_from_midi_channel(channel);

    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
        return track_info_tuple.first;

    const auto track_info = track_info_tuple.second;

    const auto status = _midi_dispatcher->connect_track_to_output(port, track_info.name, int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::connect_cc_to_parameter(int processor_id,
                                                           int parameter_id,
                                                           ext::MidiChannel channel,
                                                           int port,
                                                           int cc_number,
                                                           float min_range,
                                                           float max_range,
                                                           bool relative_mode)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    // TODO: Move these out, and change the API, so that the method is called with string instead of int ID?
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
        return processor_info_tuple.first;

    const auto processor_info = processor_info_tuple.second;
    const std::string processor_name = processor_info.name;

    auto parameter_info_tuple = _parameter_controller->get_parameter_info(processor_id, parameter_id);
    if(parameter_info_tuple.first != ext::ControlStatus::OK)
        return parameter_info_tuple.first;

    auto parameter_info = parameter_info_tuple.second;

    const std::string parameter_name = parameter_info.name;

    auto status = _midi_dispatcher->connect_cc_to_parameter(midi_input,
                                                            processor_name,
                                                            parameter_name,
                                                            cc_number,
                                                            min_range,
                                                            max_range,
                                                            relative_mode,
                                                            int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
        return processor_info_tuple.first;

    const auto track_info = processor_info_tuple.second;
    const std::string processor_name = track_info.name;

    const auto status = _midi_dispatcher->connect_pc_to_processor(midi_input, processor_name, int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port, bool raw_midi)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
        return track_info_tuple.first;

    const auto track_info = track_info_tuple.second;
    const std::string track_name = track_info.name;

    midi_dispatcher::MidiDispatcherStatus status;
    if(!raw_midi)
    {
        status = _midi_dispatcher->disconnect_kb_from_track(midi_input, track_name, int_channel);
    }
    else
    {
        status = _midi_dispatcher->disconnect_raw_midi_from_track(midi_input, track_name, int_channel);
    }

    if (status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port)
{
    const int int_channel = int_from_midi_channel(channel);

    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
        return track_info_tuple.first;

    const auto track_info = track_info_tuple.second;

    const auto status = _midi_dispatcher->disconnect_track_from_output(port, track_info.name, int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    // TODO: Move these out, and change the API, so that the method is called with string instead of int ID?
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
        return processor_info_tuple.first;

    const auto processor_info = processor_info_tuple.second;
    const std::string processor_name = processor_info.name;

    const auto status = _midi_dispatcher->disconnect_cc_from_parameter(midi_input,
                                                                       processor_name,
                                                                       cc_number,
                                                                       int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::disconnect_pc(int processor_id, ext::MidiChannel channel, int port)
{
    const int midi_input = port;
    const int int_channel = int_from_midi_channel(channel);

    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
        return processor_info_tuple.first;

    const auto track_info = processor_info_tuple.second;
    const std::string processor_name = track_info.name;

    const auto status = _midi_dispatcher->disconnect_pc_from_processor(midi_input, processor_name, int_channel);

    if(status == midi_dispatcher::MidiDispatcherStatus::OK)
    {
        return sushi::ext::ControlStatus::OK;
    }
    else
    {
        // TODO: Expand.
        return sushi::ext::ControlStatus::ERROR;
    }
}

ext::ControlStatus MidiController::disconnect_all_cc_from_processor(int processor_id)
{
    auto output_connections = get_cc_input_connections_for_processor(processor_id);
    if(output_connections.first == sushi::ext::ControlStatus::OK)
    {
        for (const auto& connection : output_connections.second)
        {
            const auto status = disconnect_cc(processor_id,
                                              connection.channel,
                                              connection.port,
                                              connection.cc_number);

            if(status != sushi::ext::ControlStatus::OK)
            {
                // TODO: We could do with a better error message.
                return sushi::ext::ControlStatus::ERROR;
            }
        }
    }
    else
    {
        // TODO: We could do with a better error message.
        return sushi::ext::ControlStatus::ERROR;
    }

    return sushi::ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_all_pc_from_processor(int processor_id)
{
    auto output_connections = get_pc_input_connections_for_processor(processor_id);
    if(output_connections.first == sushi::ext::ControlStatus::OK)
    {
        for (const auto& connection : output_connections.second)
        {
            const auto status = disconnect_pc(processor_id,
                                              connection.channel,
                                              connection.port);

            if(status != sushi::ext::ControlStatus::OK)
            {
                // TODO: We could do with a better error message.
                return sushi::ext::ControlStatus::ERROR;
            }
        }
    }
    else
    {
        // TODO: We could do with a better error message.
        return sushi::ext::ControlStatus::ERROR;
    }

    return sushi::ext::ControlStatus::OK;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi
