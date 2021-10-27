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
 * @brief OSC runtime control frontend
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>
#include <sstream>

#include <lo/lo_types.h>

#include "osc_utils.h"
#include "osc_frontend.h"
#include "logging.h"

namespace sushi {
namespace control_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("osc frontend");

namespace {

static void osc_error(int num, const char* msg, const char* path)
{
    if (msg && path) // Sometimes liblo passes a nullpointer for msg
    {
        SUSHI_LOG_ERROR("liblo server error {} in path {}: {}", num, path, msg);
    }
}

static int osc_send_parameter_change_event(const char* /*path*/,
                                           const char* /*types*/,
                                           lo_arg** argv,
                                           int /*argc*/,
                                           lo_message /*data*/,
                                           void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    float value = argv[0]->f;
    auto ctrl = connection->controller->parameter_controller();
    ctrl->set_parameter_value(connection->processor, connection->parameter, value);
    SUSHI_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_string_parameter_change_event(const char* /*path*/,
                                                  const char* /*types*/,
                                                  lo_arg** argv,
                                                  int /*argc*/,
                                                  lo_message /*data*/,
                                                  void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string value(&argv[0]->s);
    auto controller = connection->controller->parameter_controller();
    controller->set_string_property_value(connection->processor, connection->parameter, value);
    SUSHI_LOG_DEBUG("Sending string property {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_bypass_state_event(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       lo_message /*data*/,
                                       void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    bool isBypassed = (argv[0]->i == 0) ? false : true;
    auto controller = connection->controller->audio_graph_controller();
    controller->set_processor_bypass_state(connection->processor, isBypassed);
    SUSHI_LOG_DEBUG("Setting processor {} bypass to {}", connection->processor, isBypassed);
    return 0;
}

static int osc_send_keyboard_note_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                    lo_message /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    int note = argv[2]->i;
    float value = argv[3]->f;
    auto controller = connection->controller->keyboard_controller();

    if (event == "note_on")
    {
        controller->send_note_on(connection->processor, channel, note, value);
    }
    else if (event == "note_off")
    {
        controller->send_note_off(connection->processor, channel, note, value);
    }
    else if (event == "note_aftertouch")
    {
        controller->send_note_aftertouch(connection->processor, channel, note, value);
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized event: {}.", event);
        return 0;
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

static int osc_send_keyboard_modulation_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   lo_message /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    float value = argv[2]->f;
    auto controller = connection->controller->keyboard_controller();

    if (event == "modulation")
    {
        controller->send_modulation(connection->processor, channel, value);
    }
    else if (event == "pitch_bend")
    {
        controller->send_pitch_bend(connection->processor, channel, value);
    }
    else if (event == "aftertouch")
    {
        controller->send_aftertouch(connection->processor, channel, value);
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized event: {}.", event);
        return 0;
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

static int osc_send_program_change_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   lo_message /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    int program_id = argv[0]->i;
    auto controller = connection->controller->program_controller();
    controller->set_processor_program(connection->processor, program_id);
    return 0;
}

static int osc_set_timing_statistics_enabled(const char* /*path*/,
                                             const char* /*types*/,
                                             lo_arg** argv,
                                             int /*argc*/,
                                             lo_message /*data*/,
                                             void* user_data)
{
    auto controller = static_cast<ext::SushiControl*>(user_data)->timing_controller();
    bool is_enabled = (argv[0]->i == 0) ? false : true;
    SUSHI_LOG_DEBUG("Got request to set timing statistics enabled to {}", is_enabled);

    controller->set_timing_statistics_enabled(is_enabled);
    return 0;
}

static int osc_reset_timing_statistics(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       lo_message /*data*/,
                                       void* user_data)
{
    auto ctrl = static_cast<ext::SushiControl*>(user_data);
    auto timing_ctrl = ctrl->timing_controller();
    auto processor_ctrl = ctrl->audio_graph_controller();

    std::string output_text = std::string(&argv[0]->s);
    std::string_view type = output_text;
    if (type == "all")
    {
        auto status = timing_ctrl->reset_all_timings();
        if (status != ext::ControlStatus::OK)
        {
            SUSHI_LOG_WARNING("Failed to reset track timings of all tracks and processors");
            return 0;
        }
    }
    else if (type == "track")
    {
        std::string track_name = std::string(&argv[1]->s);

        auto [track_status, track_id] = processor_ctrl->get_track_id(track_name);
        if (track_status == ext::ControlStatus::OK)
        {
            output_text += " " + track_name;
            timing_ctrl->reset_track_timings(track_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No track with name {} available", track_name);
            return 0;
        }
    }
    else if (type == "processor")
    {
        std::string processor_name = std::string(&argv[1]->s);
        auto [processor_status, processor_id] = processor_ctrl->get_processor_id(processor_name);
        if (processor_status == ext::ControlStatus::OK)
        {
            output_text += " " + processor_name;
            timing_ctrl->reset_processor_timings(processor_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No processor with name {} available", processor_name);
            return 0;
        }
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized reset target");
        return 0;
    }
    SUSHI_LOG_DEBUG("Resetting {} timing statistics", output_text);
    return 0;
}

static int osc_set_tempo(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                lo_message /*data*/,
                                void* user_data)
{
    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    float tempo = argv[0]->f;
    SUSHI_LOG_DEBUG("Got a set tempo request to {} bpm", tempo);
    controller->set_tempo(tempo);
    return 0;
}

static int osc_set_time_signature(const char* /*path*/,
                                  const char* /*types*/,
                                  lo_arg** argv,
                                  int /*argc*/,
                                  lo_message /*data*/,
                                  void* user_data)
{
    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    int numerator = argv[0]->i;
    int denominator = argv[1]->i;
    SUSHI_LOG_DEBUG("Got a set time signature to {}/{} request", numerator, denominator);
    controller->set_time_signature({numerator, denominator});
    return 0;
}

static int osc_set_playing_mode(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                lo_message /*data*/,
                                void* user_data)
{
    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    std::string_view mode_str(&argv[0]->s);
    ext::PlayingMode mode;
    if (mode_str == "playing")
    {
        mode = ext::PlayingMode::PLAYING;
    }
    else if (mode_str == "stopped")
    {
        mode = ext::PlayingMode::STOPPED;
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised playing mode \"{}\" received", mode_str);
        return 0;
    }

    SUSHI_LOG_DEBUG("Got a set playing mode {} request", mode_str);
    controller->set_playing_mode(mode);
    return 0;
}

static int osc_set_tempo_sync_mode(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   lo_message /*data*/,
                                   void* user_data)
{
    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    std::string_view mode_str(&argv[0]->s);
    ext::SyncMode mode;
    if (mode_str == "internal")
    {
        mode = ext::SyncMode::INTERNAL;
    }
    else if (mode_str == "ableton_link")
    {
        mode = ext::SyncMode::LINK;
    }
    else if (mode_str == "midi")
    {
        mode = ext::SyncMode::MIDI;
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised sync mode \"{}\" received", mode_str);
        return 0;
    }
    SUSHI_LOG_DEBUG("Got a set sync mode to {} request", mode_str);
    controller->set_sync_mode(mode);
    return 0;
}

}; // anonymous namespace

OSCFrontend::OSCFrontend(engine::BaseEngine* engine,
                         ext::SushiControl* controller,
                         int receive_port,
                         int send_port) : BaseControlFrontend(engine, EventPosterId::OSC_FRONTEND),
                                          _receive_port(receive_port),
                                          _send_port(send_port),
                                          _controller(controller),
                                          _graph_controller(controller->audio_graph_controller()),
                                          _param_controller(controller->parameter_controller())
{}

ControlFrontendStatus OSCFrontend::init()
{
    std::stringstream port_stream;
    port_stream << _receive_port;
    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
    if (_osc_server == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to set up OSC server, Port likely in use ({})", _receive_port);
        return ControlFrontendStatus::INTERFACE_UNAVAILABLE;
    }

    std::stringstream send_port_stream;
    send_port_stream << _send_port;
    _osc_out_address = lo_address_new(nullptr, send_port_stream.str().c_str());

    _setup_engine_control();
    _osc_initialized = true;
    _event_dispatcher->subscribe_to_parameter_change_notifications(this);
    _event_dispatcher->subscribe_to_engine_notifications(this);

    return ControlFrontendStatus::OK;
}

OSCFrontend::~OSCFrontend()
{
    if (_running)
    {
        _stop_server();
    }

    if (_osc_initialized) // These are set up in init, where also _osc_initialized is set to true.
    {
        _event_dispatcher->unsubscribe_from_parameter_change_notifications(this);
        _event_dispatcher->unsubscribe_from_engine_notifications(this);
        _osc_initialized = false;
        lo_server_thread_free(_osc_server);
        lo_address_free(_osc_out_address);
    }
}

std::pair<OscConnection*, std::string> OSCFrontend::_create_parameter_connection(const std::string& processor_name,
                                                                                 const std::string& parameter_name)
{
    std::string osc_path = "/parameter/";
    auto [processor_status, processor_id] = _graph_controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    auto [parameter_status, parameter_id] = _param_controller->get_parameter_id(processor_id, parameter_name);
    if (parameter_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    osc_path = osc_path + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);
    auto connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    connection->controller = _controller;
    return {connection, osc_path};
}

bool OSCFrontend::connect_to_parameter(const std::string& processor_name,
                                       const std::string& parameter_name)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return false;
    }

    auto [connection, osc_path] = _create_parameter_connection(processor_name, parameter_name);
    if (connection == nullptr)
    {
        return false;
    }
    auto cb = lo_server_thread_add_method(_osc_server,
                                          osc_path.c_str(),
                                          "f",
                                          osc_send_parameter_change_event,
                                          connection);

    connection->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_string_parameter(const std::string& processor_name,
                                              const std::string& parameter_name)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return false;
    }

    auto [connection, osc_path] = _create_parameter_connection(processor_name, parameter_name);
    if (connection == nullptr)
    {
        return false;
    }
    auto cb = lo_server_thread_add_method(_osc_server,
                                          osc_path.c_str(),
                                          "s",
                                          osc_send_string_parameter_change_event,
                                          connection);
    connection->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
    auto [processor_status, processor_id] = _graph_controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return false;
    }
    auto [parameter_status, parameter_id] = _param_controller->get_parameter_id(processor_id, parameter_name);
    if (parameter_status != ext::ControlStatus::OK)
    {
        return false;
    }
    std::string id_string = "/parameter/" + osc::make_safe_path(processor_name) + "/" +
                                            osc::make_safe_path(parameter_name);

    _outgoing_connections[processor_id][parameter_id] = id_string;

    SUSHI_LOG_INFO("Added osc output from parameter {}/{}", processor_name, parameter_name);
    return true;
}

bool OSCFrontend::disconnect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
    auto [processor_status, processor_id] = _graph_controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return false;
    }
    auto [parameter_status, parameter_id] = _param_controller->get_parameter_id(processor_id, parameter_name);
    if (parameter_status != ext::ControlStatus::OK)
    {
        return false;
    }

    _outgoing_connections[processor_id].erase(parameter_id);

    return true;
}

std::pair<OscConnection*, std::string> OSCFrontend::_create_processor_connection(const std::string& processor_name,
                                                                                 const std::string& osc_path_prefix)
{
    std::string osc_path = osc_path_prefix;
    auto [processor_status, processor_id] = _graph_controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    osc_path = osc_path + osc::make_safe_path(processor_name);
    auto connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    connection->controller = _controller;
    return {connection, osc_path};
}

bool OSCFrontend::connect_to_bypass_state(const std::string& processor_name)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return false;
    }

    auto [connection, osc_path] = _create_processor_connection(processor_name, "/bypass/");
    if (connection == nullptr)
    {
        return false;
    }
    auto cb = lo_server_thread_add_method(_osc_server, osc_path.c_str(), "i", osc_send_bypass_state_event, connection);
    connection->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_kb_to_track(const std::string& track_name)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return false;
    }

    auto [connection, osc_path] = _create_processor_connection(track_name, "/keyboard_event/");
    if (connection == nullptr)
    {
        return false;
    }
    auto cb = lo_server_thread_add_method(_osc_server, osc_path.c_str(), "siif", osc_send_keyboard_note_event, connection);
    connection->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));

    auto dupl_conn = new OscConnection(*connection);
    cb = lo_server_thread_add_method(_osc_server, osc_path.c_str(), "sif", osc_send_keyboard_modulation_event, connection);
    dupl_conn->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(dupl_conn));
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_program_change(const std::string& processor_name)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return false;
    }

    auto [connection, osc_path] = _create_processor_connection(processor_name, "/program/");
    if (connection == nullptr)
    {
        return false;
    }
    auto cb = lo_server_thread_add_method(_osc_server, osc_path.c_str(), "i", osc_send_program_change_event, connection);
    connection->liblo_cb = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_processor_parameters(const std::string& processor_name, int processor_id)
{
    auto [parameters_status, parameters] = _param_controller->get_processor_parameters(processor_id);
    if (parameters_status != ext::ControlStatus::OK)
    {
        return false;
    }
    for (auto& param : parameters)
    {
        if (param.type == ext::ParameterType::FLOAT ||
            param.type == ext::ParameterType::INT ||
            param.type == ext::ParameterType::BOOL)
        {
            connect_to_parameter(processor_name, param.name);
        }
        if (param.type == ext::ParameterType::STRING_PROPERTY)
        {
            connect_to_string_parameter(processor_name, param.name);
        }
    }
    return true;
}

bool OSCFrontend::connect_from_processor_parameters(const std::string& processor_name, int processor_id)
{
    auto [parameters_status, parameters] = _param_controller->get_processor_parameters(processor_id);
    if (parameters_status != ext::ControlStatus::OK)
    {
        return false;
    }
    for (auto& param : parameters)
    {
        if (param.type == ext::ParameterType::FLOAT || param.type == ext::ParameterType::INT || param.type == ext::ParameterType::BOOL)
        {
            connect_from_parameter(processor_name, param.name);
        }
    }
    return true;
}

bool OSCFrontend::disconnect_from_processor_parameters(const std::string& processor_name, int processor_id)
{
    auto [parameters_status, parameters] = _param_controller->get_processor_parameters(processor_id);
    if (parameters_status != ext::ControlStatus::OK)
    {
        return false;
    }
    for (auto& param : parameters)
    {
        disconnect_from_parameter(processor_name, param.name);
    }
    return true;
}

void OSCFrontend::connect_to_all()
{
    auto tracks = _graph_controller->get_all_tracks();
    for (auto& track : tracks)
    {
        connect_to_processor_parameters(track.name, track.id);
        auto [processors_status, processors] = _graph_controller->get_track_processors(track.id);
        if (processors_status != ext::ControlStatus::OK)
        {
            return;
        }
        for (auto& processor : processors)
        {
            connect_to_processor_parameters(processor.name, processor.id);
            if (processor.program_count > 0)
            {
                connect_to_program_change(processor.name);
            }
            connect_to_bypass_state(processor.name);
        }
        connect_kb_to_track(track.name);
    }
}

void OSCFrontend::connect_from_all_parameters()
{
    auto tracks = _graph_controller->get_all_tracks();
    for (auto& track : tracks)
    {
        connect_from_processor_parameters(track.name, track.id);
        auto [processors_status, processors] = _graph_controller->get_track_processors(track.id);
        if (processors_status != ext::ControlStatus::OK)
        {
            return;
        }
        for (auto& processor : processors)
        {
            connect_from_processor_parameters(processor.name, processor.id);
        }
    }
}

void OSCFrontend::disconnect_from_all_parameters()
{
    auto tracks = _graph_controller->get_all_tracks();
    for (auto& track : tracks)
    {
        disconnect_from_processor_parameters(track.name, track.id);
        auto [processors_status, processors] = _graph_controller->get_track_processors(track.id);
        if (processors_status == ext::ControlStatus::OK)
        {
            for (auto& processor : processors)
            {
                disconnect_from_processor_parameters(processor.name, processor.id);
            }
        }
    }
}

int OSCFrontend::process(Event* event)
{
    assert(_osc_initialized);

    if (event->is_parameter_change_notification())
    {
        _handle_param_change_notification(static_cast<ParameterChangeNotificationEvent*>(event));
    }
    else if (event->is_engine_notification())
    {
        _handle_engine_notification(static_cast<EngineNotificationEvent*>(event));
    }
    // Return statuses for notifications are not handled, so just return ok.
    return EventStatus::HANDLED_OK;
}

int OSCFrontend::receive_port() const
{
    return _receive_port;
}

int OSCFrontend::send_port() const
{
    return _send_port;
}

std::vector<std::string> OSCFrontend::get_enabled_parameter_outputs()
{
    auto outputs = std::vector<std::string>();

    for (const auto& connectionPair : _outgoing_connections)
    {
        for (const auto& connection : connectionPair.second)
        {
            outputs.push_back(connection.second);
        }
    }

    return outputs;
}

void OSCFrontend::_setup_engine_control()
{
    lo_server_thread_add_method(_osc_server, "/engine/set_tempo", "f", osc_set_tempo, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_time_signature", "ii", osc_set_time_signature, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_playing_mode", "s", osc_set_playing_mode, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_sync_mode", "s", osc_set_tempo_sync_mode, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_timing_statistics_enabled", "i", osc_set_timing_statistics_enabled, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/reset_timing_statistics", "s", osc_reset_timing_statistics, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/reset_timing_statistics", "ss", osc_reset_timing_statistics, this->_controller);
}

void OSCFrontend::_completion_callback(Event* event, int return_status)
{
    SUSHI_LOG_DEBUG("EngineEvent {} completed with status {}({})", event->id(), return_status == 0 ? "ok" : "failure", return_status);
}

void OSCFrontend::_start_server()
{
    assert(_osc_initialized);

    _running.store(true);

    int ret = lo_server_thread_start(_osc_server);
    if (ret < 0)
    {
        SUSHI_LOG_ERROR("Error {} while starting OSC server thread", ret);
    }
}

void OSCFrontend::_stop_server()
{
    assert(_osc_initialized);

    _running.store(false);
    int ret = lo_server_thread_stop(_osc_server);
    if (ret < 0)
    {
        SUSHI_LOG_ERROR("Error {} while stopping OSC server thread", ret);
    }
}

bool OSCFrontend::_remove_processor_connections(ObjectId processor_id)
{
    assert(_osc_initialized);

    int count = 0;
    for (const auto& c : _connections)
    {
        if (c->processor == processor_id)
        {
            lo_server_thread_del_lo_method(_osc_server, c->liblo_cb);
            count++;
        }
    }
    _connections.erase(std::remove_if(_connections.begin(),
                                      _connections.end(),
                                      [&](const auto& c) { return c->processor == processor_id; }),
                       _connections.end());

    count += _outgoing_connections.erase(static_cast<ObjectId>(processor_id));

    SUSHI_LOG_ERROR_IF(count == 0, "Failed to remove any connections for processor {}", processor_id);
    return count > 0;
}

void OSCFrontend::_handle_engine_notification(const EngineNotificationEvent* event)
{
    if (event->is_clipping_notification())
    {
        _handle_clipping_notification(static_cast<const ClippingNotificationEvent*>(event));
    }
    else if (event->is_audio_graph_notification())
    {
        _handle_audio_graph_notification(static_cast<const AudioGraphNotificationEvent*>(event));
    }
}

void OSCFrontend::_handle_param_change_notification(const ParameterChangeNotificationEvent* event)
{
    const auto& node = _outgoing_connections.find(event->processor_id());
    if (node != _outgoing_connections.end())
    {
        const auto& param_node = node->second.find(event->parameter_id());
        if (param_node != node->second.end())
        {
            lo_send(_osc_out_address, param_node->second.c_str(), "f", event->float_value());
            SUSHI_LOG_DEBUG("Sending parameter change from processor: {}, parameter: {}, value: {}", event->processor_id(), event->parameter_id(), event->float_value());
        }
    }
}

void OSCFrontend::_handle_clipping_notification(const ClippingNotificationEvent* event)
{
    if (event->channel_type() == ClippingNotificationEvent::ClipChannelType::INPUT)
    {
        lo_send(_osc_out_address, "/engine/input_clip_notification", "i", event->channel());
    }
    else if (event->channel_type() == ClippingNotificationEvent::ClipChannelType::OUTPUT)
    {
        lo_send(_osc_out_address, "/engine/output_clip_notification", "i", event->channel());
    }
}

void OSCFrontend::_handle_audio_graph_notification(const AudioGraphNotificationEvent* event)
{
    switch(event->action())
    {
        case AudioGraphNotificationEvent::Action::PROCESSOR_CREATED:
        {
            SUSHI_LOG_DEBUG("Received a PROCESSOR_CREATED notification for processor {}", event->processor());
            auto [status, info] = _graph_controller->get_processor_info(event->processor());
            if (status == ext::ControlStatus::OK)
            {
                connect_to_bypass_state(info.name);
                connect_to_program_change(info.name);
                connect_to_processor_parameters(info.name, event->processor());
                if(_connect_from_all_parameters)
                {
                    connect_from_processor_parameters(info.name, event->processor());
                }
            }
            SUSHI_LOG_ERROR_IF(status != ext::ControlStatus::OK, "Failed to get info for processor {}", event->processor());
            break;
        }

        case AudioGraphNotificationEvent::Action::TRACK_CREATED:
        {
            SUSHI_LOG_DEBUG("Received a TRACK_ADDED notification for track {}", event->track());
            auto [status, info] = _graph_controller->get_track_info(event->track());
            if (status == ext::ControlStatus::OK)
            {
                connect_kb_to_track(info.name);
                connect_to_bypass_state(info.name);
                connect_to_processor_parameters(info.name, event->track());
                if(_connect_from_all_parameters)
                {
                    connect_from_processor_parameters(info.name, event->processor());
                }
            }
            SUSHI_LOG_ERROR_IF(status != ext::ControlStatus::OK, "Failed to get info for track {}", event->track());
            break;
        }

        case AudioGraphNotificationEvent::Action::PROCESSOR_DELETED:
            SUSHI_LOG_DEBUG("Received a PROCESSOR_DELETED notification for processor {}", event->processor());
            _remove_processor_connections(event->processor());
            break;

        case AudioGraphNotificationEvent::Action::TRACK_DELETED:
            SUSHI_LOG_DEBUG("Received a TRACK_DELETED notification for processor {}", event->track());
            _remove_processor_connections(event->track());
            break;

        default:
            break;
    }
}

} // namespace control_frontend

std::string osc::make_safe_path(std::string name)
{
    // Based on which characters are invalid in the OSC Spec plus \ and "
    constexpr std::string_view INVALID_CHARS = "#*./?[]{}\"\\";
    for (char i : INVALID_CHARS)
    {
        name.erase(std::remove(name.begin(), name.end(), i), name.end());
    }
    std::replace(name.begin(), name.end(), ' ', '_');
    return name;
}

}
