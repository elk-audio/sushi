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

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {

// TODO: I don't like this being here. It works, but I should move it.
// It broke some unit tests that included .cpp files, this was a workaround.
namespace ext {
ext::MidiChannel midi_channel_from_int(int channel_int)
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
}

namespace engine {
namespace controller_impl {

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

    const auto connections = _midi_dispatcher->get_all_kb_input_connections();
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        ext::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection->input_connection.target;
        ext_connection.port = connection->port;
        ext_connection.channel = ext::midi_channel_from_int(connection->channel);
        ext_connection.raw_midi = connection->raw_midi;
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_output_connections() const
{
    std::vector<ext::MidiKbdConnection> returns;

    const auto connections = _midi_dispatcher->get_all_kb_output_connections();
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        ext::MidiKbdConnection ext_connection;
        ext_connection.track_id = connection->track_id;
        ext_connection.port = connection->port;
        ext_connection.channel = ext::midi_channel_from_int(connection->channel);
        ext_connection.raw_midi = false;
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_input_connections() const
{
    std::vector<ext::MidiCCConnection> returns;

    const auto connections = _midi_dispatcher->get_all_cc_input_connections();
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = _populate_cc_connection(*connection);
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::vector<ext::MidiPCConnection> MidiController::get_all_pc_input_connections() const
{
    std::vector<ext::MidiPCConnection> returns;

    const auto connections = _midi_dispatcher->get_all_pc_input_connections();
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = _populate_pc_connection(*connection);
        returns.emplace_back(ext_connection);
    }

    return returns;
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
MidiController::get_cc_input_connections_for_processor(int processor_id) const
{
    std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>> returns;
    returns.first = ext::ControlStatus::OK;

    const auto connections = _midi_dispatcher->get_cc_input_connections_for_processor(processor_id);
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = _populate_cc_connection(*connection);
        returns.second.emplace_back(ext_connection);
    }

    return returns;
}

std::pair<ext::ControlStatus, std::vector<ext::MidiPCConnection>>
MidiController::get_pc_input_connections_for_processor(int processor_id) const
{
    std::pair<ext::ControlStatus, std::vector<ext::MidiPCConnection>> returns;
    returns.first = ext::ControlStatus::OK;

    const auto connections = _midi_dispatcher->get_pc_input_connections_for_processor(processor_id);
    for(auto connection = connections.begin(); connection != connections.end(); ++connection)
    {
        auto ext_connection = _populate_pc_connection(*connection);
        returns.second.emplace_back(ext_connection);
    }

    return returns;
}

ext::ControlStatus MidiController::connect_kbd_input_to_track(int track_id,
                                                              ext::MidiChannel channel,
                                                              int port,
                                                              bool raw_midi)
{
    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string track_name = track_info_tuple.second.name;

    auto event = new KbdInputToTrackConnectionEvent(_midi_dispatcher,
                                                    track_name,
                                                    channel,
                                                    port,
                                                    raw_midi,
                                                    KbdInputToTrackConnectionEvent::Action::Connect,
                                                    IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::connect_kbd_output_from_track(int track_id,
                                                                 ext::MidiChannel channel,
                                                                 int port)
{
    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string track_name = track_info_tuple.second.name;

    auto event = new KbdOutputToTrackConnectionEvent(_midi_dispatcher,
                                                     track_name,
                                                     channel,
                                                     port,
                                                     KbdOutputToTrackConnectionEvent::Action::Connect,
                                                     IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
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
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name =  processor_info_tuple.second.name;

    const auto parameter_info_tuple = _parameter_controller->get_parameter_info(processor_id, parameter_id);
    if(parameter_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string parameter_name =parameter_info_tuple.second.name;

    auto event = new ConnectCCToParameterEvent(_midi_dispatcher,
                                               processor_name,
                                               parameter_name,
                                               channel,
                                               port,
                                               cc_number,
                                               min_range,
                                               max_range,
                                               relative_mode,
                                               IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port)
{
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name =  processor_info_tuple.second.name;

    auto event = new PCToProcessorConnectionEvent(_midi_dispatcher,
                                                  processor_name,
                                                  channel,
                                                  port,
                                                  PCToProcessorConnectionEvent::Action::Connect,
                                                  IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port, bool raw_midi)
{
    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string track_name = track_info_tuple.second.name;

    auto event = new KbdInputToTrackConnectionEvent(_midi_dispatcher,
                                                    track_name,
                                                    channel,
                                                    port,
                                                    raw_midi,
                                                    KbdInputToTrackConnectionEvent::Action::Disconnect,
                                                    IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port)
{
    const auto track_info_tuple = _graph_controller->get_track_info(track_id);
    if(track_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string track_name = track_info_tuple.second.name;

    auto event = new KbdOutputToTrackConnectionEvent(_midi_dispatcher,
                                                     track_name,
                                                     channel,
                                                     port,
                                                     KbdOutputToTrackConnectionEvent::Action::Disconnect,
                                                     IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number)
{
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name = processor_info_tuple.second.name;

    auto event = new DisconnectCCEvent(_midi_dispatcher,
                                       processor_name,
                                       channel,
                                       port,
                                       cc_number,
                                       IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_pc(int processor_id, ext::MidiChannel channel, int port)
{
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name = processor_info_tuple.second.name;

    auto event = new PCToProcessorConnectionEvent(_midi_dispatcher,
                                                  processor_name,
                                                  channel,
                                                  port,
                                                  PCToProcessorConnectionEvent::Action::Disconnect,
                                                  IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_all_cc_from_processor(int processor_id)
{
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name = processor_info_tuple.second.name;

    auto event = new DisconnectAllCCFromProcessorEvent(_midi_dispatcher,
                                                       processor_name,
                                                       IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus MidiController::disconnect_all_pc_from_processor(int processor_id)
{
    const auto processor_info_tuple = _graph_controller->get_processor_info(processor_id);
    if(processor_info_tuple.first != ext::ControlStatus::OK)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    const std::string processor_name = processor_info_tuple.second.name;

    auto event = new DisconnectAllPCFromProcessorEvent(_midi_dispatcher,
                                                       processor_name,
                                                       IMMEDIATE_PROCESS);

    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::MidiCCConnection MidiController::_populate_cc_connection(const midi_dispatcher::CC_InputConnection& connection) const
{
    ext::MidiCCConnection ext_connection;

    ext_connection.processor_id = connection.input_connection.target;
    ext_connection.parameter_id = connection.input_connection.parameter;
    ext_connection.min_range = connection.input_connection.min_range;
    ext_connection.max_range = connection.input_connection.max_range;
    ext_connection.relative_mode = connection.input_connection.relative;
    ext_connection.channel = ext::midi_channel_from_int(connection.channel);
    ext_connection.port = connection.port;
    ext_connection.cc_number = connection.cc;

    return ext_connection;
}

ext::MidiPCConnection MidiController::_populate_pc_connection(const midi_dispatcher::PC_InputConnection& connection) const
{
    ext::MidiPCConnection ext_connection;

    ext_connection.processor_id = connection.processor_id;
    ext_connection.channel = ext::midi_channel_from_int(connection.channel);
    ext_connection.port = connection.port;

    return ext_connection;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi
