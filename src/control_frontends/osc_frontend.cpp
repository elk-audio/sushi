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
    connection->controller->set_parameter_value(connection->parameter, connection->processor, value);
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

static int osc_send_keyboard_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   void* /*data*/,
                                   void* user_data)
{
    // TODO: Enable aftertouch message and midi channel
    auto connection = static_cast<OscConnection*>(user_data);
    std::string event(&argv[0]->s);
    int note = argv[1]->i;
    float value = argv[2]->f;

    if (event == "note_on")
    {
        connection->controller->send_note_on(connection->processor, 0, note, value);
    }
    else if (event == "note_off")
    {
        connection->controller->send_note_off(connection->processor, 0, note, value);
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

static int osc_add_track(const char* /*path*/,
                         const char* /*types*/,
                         lo_arg** argv,
                         int /*argc*/,
                         void* /*data*/,
                         void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string name(&argv[0]->s);
    int channels = argv[1]->i;
    SUSHI_LOG_DEBUG("Got an osc_add_track request {} {}", name, channels);
    instance->send_add_track_event(name, channels);
    return 0;
}

static int osc_delete_track(const char* /*path*/,
                            const char* /*types*/,
                            lo_arg** argv,
                            int /*argc*/,
                            void* /*data*/,
                            void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string name(&argv[0]->s);
    SUSHI_LOG_DEBUG("Got an osc_delete_track request {}", name);
    instance->send_remove_track_event(name);
    return 0;
}

static int osc_add_processor(const char* /*path*/,
                             const char* /*types*/,
                             lo_arg** argv,
                             int /*argc*/,
                             void* /*data*/,
                             void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string track(&argv[0]->s);
    std::string uid(&argv[1]->s);
    std::string name(&argv[2]->s);
    std::string file(&argv[3]->s);
    std::string type(&argv[4]->s);
    // TODO If these are eventually to be accessed by a user we must sanitize
    // the input and disallow supplying a direct library path for loading.
    SUSHI_LOG_DEBUG("Got an add_processor request {}", name);
    AddProcessorEvent::ProcessorType processor_type;
    if (type == "internal")
    {
        processor_type = AddProcessorEvent::ProcessorType::INTERNAL;
    }
    else if (type == "vst2x")
    {
        processor_type = AddProcessorEvent::ProcessorType::VST2X;
    }
    else if (type == "vst3x")
    {
        processor_type = AddProcessorEvent::ProcessorType::VST3X;
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized plugin type \"{}\"", type);
        return 0;
    }
    instance->send_add_processor_event(track, uid, name, file, processor_type);
    return 0;
}

static int osc_delete_processor(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                void* /*data*/,
                                void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string track(&argv[0]->s);
    std::string name(&argv[1]->s);
    SUSHI_LOG_DEBUG("Got a delete_processor request {} from {}", name, track);
    instance->send_remove_processor_event(track, name);
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
    std::string mode_str(&argv[0]->s);
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
    std::string mode_str(&argv[0]->s);
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

}; // anonymous namespace

OSCFrontend::OSCFrontend(engine::BaseEngine* engine,
                         int server_port,
                         int send_port) : BaseControlFrontend(engine, EventPosterId::OSC_FRONTEND),
                                          _osc_server(nullptr),
                                          _server_port(server_port),
                                          _send_port(send_port),
                                          _controller(engine->controller())
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
    setup_engine_control();
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

bool OSCFrontend::connect_to_parameter(const std::string& processor_name,
                                       const std::string& parameter_name)
{
    std::string osc_path = "/parameter/";
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
    osc_path = osc_path + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    connection->controller = _controller;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "f", osc_send_parameter_change_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_string_parameter(const std::string& processor_name,
                                              const std::string& parameter_name)
{
    std::string osc_path = "/parameter/";
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
    osc_path = osc_path + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    connection->controller = _controller;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
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

bool OSCFrontend::connect_kb_to_track(const std::string& track_name)
{
    std::string osc_path = "/keyboard_event/";
    auto [status, processor_id] = _controller->get_processor_id(track_name);
    if (status != ext::ControlStatus::OK)
    {
        return false;
    }
    osc_path = osc_path + osc::make_safe_path(track_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    connection->controller = _controller;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "sif", osc_send_keyboard_event, connection);
    SUSHI_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_program_change(const std::string& processor_name)
{
    std::string osc_path = "/program/";
    auto [processor_status, processor_id] = _controller->get_processor_id(processor_name);
    if (processor_status != ext::ControlStatus::OK)
    {
        return false;
    }
    osc_path = osc_path + osc::make_safe_path(processor_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    connection->controller = _controller;
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

void OSCFrontend::setup_engine_control()
{
    lo_server_thread_add_method(_osc_server, "/engine/add_track", "si", osc_add_track, this);
    lo_server_thread_add_method(_osc_server, "/engine/delete_track", "s", osc_delete_track, this);
    lo_server_thread_add_method(_osc_server, "/engine/add_processor", "sssss", osc_add_processor, this);
    lo_server_thread_add_method(_osc_server, "/engine/delete_processor", "ss", osc_delete_processor, this);
    lo_server_thread_add_method(_osc_server, "/engine/set_tempo", "f", osc_set_tempo, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_time_signature", "ii", osc_set_time_signature, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_playing_mode", "s", osc_set_playing_mode, this->_controller);
    lo_server_thread_add_method(_osc_server, "/engine/set_sync_mode", "s", osc_set_tempo_sync_mode, this->_controller);
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
