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
                                           void* /*data*/,
                                           void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    float value = argv[0]->f;
    connection->controller->set_parameter_value(connection->processor, connection->parameter, value);
    SUSHI_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_string_parameter_change_event(const char* /*path*/,
                                                  const char* /*types*/,
                                                  lo_arg** argv,
                                                  int /*argc*/,
                                                  void* /*data*/,
                                                  void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string value(&argv[0]->s);
    connection->controller->set_string_property_value(connection->processor, connection->parameter, value);
    SUSHI_LOG_DEBUG("Sending string property {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_bypass_state_event(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       void* /*data*/,
                                       void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    bool isBypassed = (argv[0]->i == 0) ? false : true;
    connection->controller->set_processor_bypass_state(connection->processor, isBypassed);
    SUSHI_LOG_DEBUG("Setting processor {} bypass to {}", connection->processor, isBypassed);
    return 0;
}                            

static int osc_send_keyboard_note_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   void* /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    int note = argv[2]->i;
    float value = argv[3]->f;

    if (event == "note_on")
    {
        connection->controller->send_note_on(connection->processor, channel, note, value);
    }
    else if (event == "note_off")
    {
        connection->controller->send_note_off(connection->processor, channel, note, value);
    }
    else if (event == "note_aftertouch")
    {
        connection->controller->send_note_aftertouch(connection->processor, channel, note, value);
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
                                   void* /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    float value = argv[2]->f;

    if (event == "modulation")
    {
        connection->controller->send_modulation(connection->processor, channel, value);
    }
    else if (event == "pitch_bend")
    {
        connection->controller->send_pitch_bend(connection->processor, channel, value);
    }
    else if (event == "aftertouch")
    {
        connection->controller->send_aftertouch(connection->processor, channel, value);
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
                                   void* /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    int program_id = argv[0]->i;
    connection->controller->set_processor_program(connection->processor, program_id);
    return 0;
}

static int osc_set_timing_statistics_enabled(const char* /*path*/,
                                             const char* /*types*/,
                                             lo_arg** argv,
                                             int /*argc*/,
                                             void* /*data*/,
                                             void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    bool is_enabled = (argv[0]->i == 0) ? false : true;
    SUSHI_LOG_DEBUG("Got request to set timing statistics enabled to {}", is_enabled);
    instance->set_timing_statistics_enabled(is_enabled);
    return 0;
}

static int osc_reset_timing_statistics(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       void* /*data*/,
                                       void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    std::string output_text = std::string(&argv[0]->s);
    std::string_view type = output_text;
    if (type == "all")
    {
        auto status = instance->reset_all_timings();
        if (status != ext::ControlStatus::OK)
        {
            SUSHI_LOG_WARNING("Failed to reset track timings of all tracks and processors");
            return 0;
        }
    }
    else if (type == "track")
    {
        std::string track_name = std::string(&argv[1]->s);
        auto [track_status, track_id] = instance->get_track_id(track_name);
        if (track_status == ext::ControlStatus::OK)
        {
            output_text += " " + track_name;
            instance->reset_track_timings(track_id);
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
        auto [processor_status, processor_id] = instance->get_processor_id(processor_name);
        if (processor_status == ext::ControlStatus::OK)
        {
            output_text += " " + processor_name;
            instance->reset_processor_timings(processor_id);
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
                                void* /*data*/,
                                void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    float tempo = argv[0]->f;
    SUSHI_LOG_DEBUG("Got a set tempo request to {} bpm", tempo);
    instance->set_tempo(tempo);
    return 0;
}

static int osc_set_time_signature(const char* /*path*/,
                                  const char* /*types*/,
                                  lo_arg** argv,
                                  int /*argc*/,
                                  void* /*data*/,
                                  void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    int numerator = argv[0]->i;
    int denominator = argv[1]->i;
    SUSHI_LOG_DEBUG("Got a set time signature to {}/{} request", numerator, denominator);
    instance->set_time_signature({numerator, denominator});
    return 0;
}

static int osc_set_playing_mode(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                void* /*data*/,
                                void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
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
    instance->set_playing_mode(mode);
    return 0;
}

static int osc_set_tempo_sync_mode(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   void* /*data*/,
                                   void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
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
    instance->set_sync_mode(mode);
    return 0;
}

static int osc_add_processor_to_track(const char* /*path*/,
                                      const char* /*types*/,
                                      lo_arg** argv,
                                      int argc,
                                      void* /*data*/,
                                      void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    std::string name(&argv[0]->s);
    std::string uid(&argv[1]->s);
    std::string file(&argv[2]->s);
    std::string type_str(&argv[3]->s);
    std::string track(&argv[4]->s);
    std::string before;
    bool add_to_back = true;

    if (argc > 5)
    {
        before = std::string(&argv[5]->s);
        add_to_back = false;
    }

    auto type = ext::PluginType::INTERNAL;
    if (type_str == "vst2x")
    {
        type = ext::PluginType::VST2X;
    }
    else if (type_str == "vst3x")
    {
        type = ext::PluginType::VST3X;
    }
    else if (type_str == "lv2")
    {
        type = ext::PluginType::LV2;
    }
    else if (type_str != "internal")
    {
        SUSHI_LOG_INFO("Unrecognised Plugin type \"{}\" received", type_str);
        return 0;
    }

    SUSHI_LOG_DEBUG("Got a create processor on track request");
    auto [status, track_id] = instance->get_processor_id(track);
    if (status != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up Track {}", track);
        return 0;
    }
    if (add_to_back)
    {
        instance->create_processor_on_track(name, uid, file, type, track_id, std::nullopt);
    }
    else
    {
        auto [status, before_id] = instance->get_processor_id(before);
        if (status != ext::ControlStatus::OK)
        {
            SUSHI_LOG_WARNING("Error looking up processor {}", before);
            return 0;
        }
        instance->create_processor_on_track(name, uid, file, type, track_id, before_id);
    }
    return 0;
}

static int osc_move_processor(const char* /*path*/,
                              const char* /*types*/,
                              lo_arg** argv,
                              int argc,
                              void* /*data*/,
                              void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    std::string processor(&argv[0]->s);
    std::string source_track(&argv[1]->s);
    std::string dest_track(&argv[2]->s);
    std::string before;
    bool add_to_back = true;

    if (argc > 3)
    {
        before = std::string_view(&argv[3]->s);
        add_to_back = false;
    }

    SUSHI_LOG_DEBUG("Got a move processor on track request");

    auto [status_proc, proc_id] = instance->get_processor_id(processor);
    if (status_proc != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up processor {}", processor);
        return 0;
    }

    auto [status_src, src_id] = instance->get_processor_id(source_track);
    if (status_src != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up source track {}", source_track);
        return 0;
    }

    auto [status_dest, dest_id] = instance->get_processor_id(dest_track);
    if (status_dest != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up destination track {}", source_track);
        return 0;
    }

    if (add_to_back)
    {
        instance->move_processor_on_track(proc_id, src_id, dest_id, std::nullopt);
    }
    else
    {
        auto [status, before_id] = instance->get_processor_id(before);
        if (status != ext::ControlStatus::OK)
        {
            SUSHI_LOG_WARNING("Error looking up processor {}", before);
            return 0;
        }
        instance->move_processor_on_track(proc_id, src_id, dest_id, before_id);
    }
    return 0;
}

static int osc_delete_processor(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                void* /*data*/,
                                void* user_data)
{
    auto instance = static_cast<ext::SushiControl*>(user_data);
    std::string name(&argv[0]->s);
    std::string track(&argv[1]->s);

    SUSHI_LOG_DEBUG("Got a delete processor on track request");
    auto [status, proc_id] = instance->get_processor_id(name);
    if (status != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up Processor {}", name);
        return 0;
    }
    auto [track_status, track_id] = instance->get_processor_id(track);
    if (track_status != ext::ControlStatus::OK)
    {
        SUSHI_LOG_WARNING("Error looking up Track {}", name);
        return 0;
    }
    instance->delete_processor_from_track(proc_id, track_id);
    return 0;
}

}; // anonymous namespace

OSCFrontend::OSCFrontend(engine::BaseEngine* engine,
                         ext::SushiControl* controller,
                         int server_port,
                         int send_port) : BaseControlFrontend(engine, EventPosterId::OSC_FRONTEND),
                                          _osc_server(nullptr),
                                          _server_port(server_port),
                                          _send_port(send_port),
                                          _controller(controller)
{}

ControlFrontendStatus OSCFrontend::init()
{
    std::stringstream port_stream;
    port_stream << _server_port;
    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
    if (_osc_server == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to set up OSC server, Port likely in use");
        return ControlFrontendStatus::INTERFACE_UNAVAILABLE;
    }

    std::stringstream send_port_stream;
    send_port_stream << _send_port;
    _osc_out_address = lo_address_new(nullptr, send_port_stream.str().c_str());
    _setup_engine_control();
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
    lo_server_thread_free(_osc_server);
    lo_address_free(_osc_out_address);
    _event_dispatcher->unsubscribe_from_parameter_change_notifications(this);
    _event_dispatcher->unsubscribe_from_engine_notifications(this);
}

std::pair<OscConnection*, std::string> OSCFrontend::_create_parameter_connection(const std::string& processor_name,
                                                                      const std::string& parameter_name)
{
    std::string osc_path = "/parameter/";
    auto [processor_status, processor_id] = _controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    auto [parameter_status, parameter_id] = _controller->get_parameter_id(processor_id, parameter_name);
    if (parameter_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    osc_path = osc_path + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    connection->controller = _controller;
    return {connection, osc_path};
} 

bool OSCFrontend::connect_to_parameter(const std::string& processor_name,
                                       const std::string& parameter_name)
{
    auto [connection, osc_path] = _create_parameter_connection(processor_name, parameter_name);
    if (connection == nullptr)
    {
        return false;
    }
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "f", osc_send_parameter_change_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_string_parameter(const std::string& processor_name,
                                              const std::string& parameter_name)
{
    auto [connection, osc_path] = _create_parameter_connection(processor_name, parameter_name);
    if (connection == nullptr)
    {
        return false;
    }
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "s", osc_send_string_parameter_change_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
    auto [processor_status, processor_id] = _controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return false;
    }
    auto [parameter_status, parameter_id] = _controller->get_parameter_id(processor_id, parameter_name);
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

std::pair<OscConnection*, std::string> OSCFrontend::_create_processor_connection(const std::string& processor_name,
                                                                                const std::string& osc_path_prefix)
{
    std::string osc_path = osc_path_prefix;
    auto [processor_status, processor_id] = _controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return {nullptr, ""};
    }
    osc_path = osc_path + osc::make_safe_path(processor_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    connection->controller = _controller;
    return {connection, osc_path};
}

bool OSCFrontend::connect_to_bypass_state(const std::string& processor_name)
{
    auto [connection, osc_path] = _create_processor_connection(processor_name, "/bypass/");
    if (connection == nullptr)
    {
        return false;
    }
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "i", osc_send_bypass_state_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}


bool OSCFrontend::connect_kb_to_track(const std::string& track_name)
{
    auto [connection, osc_path] = _create_processor_connection(track_name, "/keyboard_event/");
    if (connection == nullptr)
    {
        return false;
    }
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "siif", osc_send_keyboard_note_event, connection);
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "sif", osc_send_keyboard_modulation_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_program_change(const std::string& processor_name)
{
    auto [connection, osc_path] = _create_processor_connection(processor_name, "/program/");
    if (connection == nullptr)
    {
        return false;
    }
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "i", osc_send_program_change_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

void OSCFrontend::connect_all()
{
    auto tracks = _controller->get_tracks();
    for (auto& track : tracks)
    {
        auto [processors_status, processors] = _controller->get_track_processors(track.id);
        if (processors_status != ext::ControlStatus::OK)
        {
            return;
        }
        for (auto& processor : processors)
        {
            auto [parameters_status, parameters] = _controller->get_processor_parameters(processor.id);
            if (parameters_status != ext::ControlStatus::OK)
            {
                return;
            }
            for (auto& param : parameters)
            {
                if (param.type == ext::ParameterType::FLOAT || param.type == ext::ParameterType::INT || param.type == ext::ParameterType::BOOL)
                {
                    connect_to_parameter(processor.name, param.name);
                    connect_from_parameter(processor.name, param.name);
                }
                if (param.type == ext::ParameterType::STRING_PROPERTY)
                {
                    connect_to_string_parameter(processor.name, param.name);
                }
            }
            if (processor.program_count > 0)
            {
                connect_to_program_change(processor.name);
            }
            connect_to_bypass_state(processor.name);
        }
        connect_kb_to_track(track.name);
    }
}

void OSCFrontend::_start_server()
{
    _running.store(true);

    int ret = lo_server_thread_start(_osc_server);
    if (ret < 0)
    {
        SUSHI_LOG_ERROR("Error {} while starting OSC server thread", ret);
    }
}

void OSCFrontend::_stop_server()
{
    _running.store(false);
    int ret = lo_server_thread_stop(_osc_server);
    if (ret < 0)
    {
        SUSHI_LOG_ERROR("Error {} while stopping OSC server thread", ret);
    }
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
    lo_server_thread_add_method(_osc_server, "/engine/add_processor_to_track", "sssss", osc_add_processor_to_track, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/add_processor_to_track", "ssssss", osc_add_processor_to_track, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/move_processor_on_track", "sss", osc_move_processor, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/move_processor_on_track", "ssss", osc_move_processor, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/delete_processor_from_track", "ss", osc_delete_processor, this->_controller);
}

int OSCFrontend::process(Event* event)
{
    if (event->is_parameter_change_notification())
    {
        auto typed_event = static_cast<ParameterChangeNotificationEvent*>(event);
        const auto& node = _outgoing_connections.find(typed_event->processor_id());
        if (node != _outgoing_connections.end())
        {
            const auto& param_node = node->second.find(typed_event->parameter_id());
            if (param_node != node->second.end())
            {
                lo_send(_osc_out_address, param_node->second.c_str(), "f", typed_event->float_value());
                SUSHI_LOG_DEBUG("Sending parameter change from processor: {}, parameter: {}, value: {}", typed_event->processor_id(), typed_event->parameter_id(), typed_event->float_value());
            }
        }
        return EventStatus::HANDLED_OK;
    }
    if (event->is_engine_notification())
    {
        // TODO - Currently the only engine notification event so direct casting works
        auto typed_event = static_cast<ClippingNotificationEvent*>(event);
        if (typed_event->channel_type() == ClippingNotificationEvent::ClipChannelType::INPUT)
        {
            lo_send(_osc_out_address, "/engine/input_clip_notification", "i", typed_event->channel());
        }
        else if (typed_event->channel_type() == ClippingNotificationEvent::ClipChannelType::OUTPUT)
        {
            lo_send(_osc_out_address, "/engine/output_clip_notification", "i", typed_event->channel());
        }
    }
    return EventStatus::NOT_HANDLED;
}

void OSCFrontend::_completion_callback(Event* event, int return_status)
{
    SUSHI_LOG_DEBUG("EngineEvent {} completed with status {}({})", event->id(), return_status == 0 ? "ok" : "failure", return_status);
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
