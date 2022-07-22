/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>
#include <sstream>

#include "osc_utils.h"
#include "osc_frontend.h"
#include "logging.h"

namespace sushi {
namespace control_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("osc frontend");

bool OscState::auto_enable_outputs() const
{
    return _auto_enable_outputs;
}

void OscState::set_auto_enable_outputs(bool value)
{
    _auto_enable_outputs = value;
}

const std::vector<std::pair<std::string, std::vector<ObjectId>>>& OscState::enabled_outputs() const
{
    return _enabled_outputs;
}

void OscState::add_enabled_outputs(const std::string& processor_name,
                                   const std::vector<ObjectId>& enabled_parameters)
{
    _enabled_outputs.emplace_back(processor_name, enabled_parameters);
}

void OscState::add_enabled_outputs(std::string&& processor_name,
                                   std::vector<ObjectId>&& enabled_parameters)
{
    _enabled_outputs.emplace_back(processor_name, enabled_parameters);
}

OSCFrontend::OSCFrontend(engine::BaseEngine* engine,
                         ext::SushiControl* controller,
                         osc::BaseOscMessenger* osc_interface) : BaseControlFrontend(engine, EventPosterId::OSC_FRONTEND),
                                          _controller(controller),
                                          _graph_controller(controller->audio_graph_controller()),
                                          _param_controller(controller->parameter_controller()),
                                          _processor_container(_engine->processor_container()),
                                          _osc(osc_interface)
{}

OSCFrontend::~OSCFrontend()
{
    if (_running)
    {
        _stop_server();
    }

    if (_osc_initialized) // These are set up in init, where also _osc_initialized is set to true.
    {
        _osc->delete_method(_set_tempo_cp);
        _osc->delete_method(_set_time_signature_cb);
        _osc->delete_method(_set_playing_mode_cb);
        _osc->delete_method(_set_sync_mode_cb);
        _osc->delete_method(_set_timing_statistics_enabled_cb);
        _osc->delete_method(_reset_timing_statistics_s_cb);
        _osc->delete_method(_reset_timing_statistics_ss_cb);

        _event_dispatcher->unsubscribe_from_parameter_change_notifications(this);
        _event_dispatcher->unsubscribe_from_engine_notifications(this);
        _osc_initialized = false;
    }
}

ControlFrontendStatus OSCFrontend::init()
{
    bool status = _osc->init();

    if (status == false)
    {
        return ControlFrontendStatus::INTERFACE_UNAVAILABLE;
    }

    _setup_engine_control();
    _osc_initialized = true;
    _event_dispatcher->subscribe_to_parameter_change_notifications(this);
    _event_dispatcher->subscribe_to_engine_notifications(this);

    return ControlFrontendStatus::OK;
}

bool OSCFrontend::connect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
    auto processor = _processor_container->processor(processor_name);
    if (processor)
    {
        auto parameter = processor->parameter_from_name(parameter_name);
        if (parameter)
        {
            _connect_from_parameter(processor_name, parameter_name, processor->id(), parameter->id());
            return true;
        }
    }
    return false;
}

bool OSCFrontend::disconnect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
    auto processor = _processor_container->processor(processor_name);
    if (processor)
    {
        auto parameter = processor->parameter_from_name(parameter_name);
        if (parameter)
        {
            _outgoing_connections[processor->id()].erase(parameter->id());
            return true;
        }
    }
    return false;
}

bool OSCFrontend::connect_from_processor_parameters(const std::string& processor_name)
{
    auto processor = _processor_container->processor(processor_name);
    if (processor)
    {
        for (auto& param: processor->all_parameters())
        {
            auto type = param->type();
            if (type == ParameterType::FLOAT || type == ParameterType::INT || type == ParameterType::BOOL)
            {
                _connect_from_parameter(processor_name, param->name(), processor->id(), param->id());
            }
        }
    }
    return true;
}

void OSCFrontend::connect_from_all_parameters()
{
    auto processors = _processor_container->all_processors();
    for (auto& processor : processors)
    {
        connect_from_processor_parameters(processor->name());
    }
}

void OSCFrontend::disconnect_from_all_parameters()
{
    _outgoing_connections.clear();
}

std::vector<std::string> OSCFrontend::get_enabled_parameter_outputs()
{
    auto outputs = std::vector<std::string>();

    for (const auto& connection_pair : _outgoing_connections)
    {
        for (const auto& connection : connection_pair.second)
        {
            outputs.push_back(connection.second);
        }
    }

    return outputs;
}

int OSCFrontend::process(Event* event)
{
    assert(_osc_initialized);

    if (event->is_parameter_change_notification())
    {
        _handle_param_change_notification(static_cast<ParameterChangeNotificationEvent*>(event));
    }
    else if (event->is_property_change_notification())
    {
        _handle_property_change_notification(static_cast<PropertyChangeNotificationEvent*>(event));
    }
    else if (event->is_engine_notification())
    {
        _handle_engine_notification(static_cast<EngineNotificationEvent*>(event));
    }
    // Return statuses for notifications are not handled, so just return ok.
    return EventStatus::HANDLED_OK;
}

std::string OSCFrontend::send_ip() const
{
    return _osc->send_ip();
}

int OSCFrontend::send_port() const
{
    return _osc->send_port();
}

int OSCFrontend::receive_port() const
{
    return _osc->receive_port();
}

OscState OSCFrontend::save_state() const
{
    OscState state;
    state.set_auto_enable_outputs(_connect_from_all_parameters);

    /* Only the outgoing connections are saved as those can be configured manually,
     * incoming osc messages are always connected to all parameters of all processors */
    for (const auto& connection_pair : _outgoing_connections)
    {
        std::vector<ObjectId> enabled_params;
        for (const auto& connection : connection_pair.second)
        {
            enabled_params.push_back(connection.first);
        }
        if (enabled_params.size() > 0)
        {
            auto processor = _processor_container->processor(connection_pair.first);
            if (processor)
            {
                state.add_enabled_outputs(std::string(processor->name()), std::move(enabled_params));
            }
            SUSHI_LOG_ERROR_IF(!processor, "Processor {}, was not found when saving state");
        }
    }

    return state;
}

void OSCFrontend::set_state(const OscState& state)
{
    _outgoing_connections.clear();
    _skip_outputs.clear();

    _connect_from_all_parameters = state.auto_enable_outputs();

    for (auto connections : state.enabled_outputs())
    {
        auto processor = _processor_container->processor(connections.first);
        if (!processor)
        {
            SUSHI_LOG_ERROR("Processor {} not found when restoring outgoing connections from state");
            continue;
        }
        for (auto param_id : connections.second)
        {
            auto param_info = processor->parameter_from_id(param_id);
            if (param_info)
            {
                _connect_from_parameter(connections.first, param_info->name(), processor->id(), param_id);
            }
        }
        if (_connect_from_all_parameters)
        {
            /* This is so that when we later receive an asynchronous PROCESSOR_ADDED event, we
             * should not add all parameter from this plugin. */
            _skip_outputs[processor->id()] = true;
        }
    }
}

void OSCFrontend::_connect_from_parameter(const std::string& processor_name,
                                          const std::string& parameter_name,
                                          ObjectId processor_id,
                                          ObjectId parameter_id)
{
    std::string id_string = "/parameter/" + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);

    _outgoing_connections[processor_id][parameter_id] = id_string;

    SUSHI_LOG_DEBUG("Added osc output from parameter {}/{}", processor_name, parameter_name);
}




std::pair<OscConnection*, std::string> OSCFrontend::_create_processor_connection(const std::string& processor_name,
                                                                                 ObjectId processor_id,
                                                                                 const std::string& osc_path_prefix)
{
    std::string osc_path = osc_path_prefix + osc::make_safe_path(processor_name);
    auto connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    connection->controller = _controller;
    return {connection, osc_path};
}


OscConnection* OSCFrontend::_connect_kb_to_track(const Processor* processor)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return nullptr;
    }

    auto [connection, osc_path] = _create_processor_connection(processor->name(), processor->id(), "/keyboard_event/");
    if (connection == nullptr)
    {
        return nullptr;
    }

    auto cb = _osc->add_method(osc_path.c_str(), "siif", osc::OscMethodType::SEND_KEYBOARD_NOTE_EVENT, connection);
    connection->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));

    auto dupl_conn = new OscConnection(*connection);
    cb = _osc->add_method(osc_path.c_str(), "sif", osc::OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, connection);
    dupl_conn->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(dupl_conn));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);

    return connection;
}

OscConnection* OSCFrontend::_connect_to_bypass_state(const Processor* processor)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return nullptr;
    }

    auto [connection, osc_path] = _create_processor_connection(processor->name(), processor->id(), "/bypass/");
    if (connection == nullptr)
    {
        return nullptr;
    }

    auto cb = _osc->add_method(osc_path.c_str(), "i", osc::OscMethodType::SEND_BYPASS_STATE_EVENT, connection);
    connection->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);
    return connection;
}

OscConnection* OSCFrontend::_connect_to_program_change(const Processor* processor)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return nullptr;
    }

    auto [connection, osc_path] = _create_processor_connection(processor->name(), processor->id(), "/program/");
    if (connection == nullptr)
    {
        return nullptr;
    }
    auto cb = _osc->add_method(osc_path.c_str(), "i", osc::OscMethodType::SEND_PROGRAM_CHANGE_EVENT, connection);
    connection->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);
    return connection;
}

OscConnection* OSCFrontend::_connect_to_parameter(const std::string& processor_name,
                                                  const std::string& parameter_name,
                                                  ObjectId processor_id,
                                                  ObjectId parameter_id)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return nullptr;
    }

    std::string osc_path = "/parameter/" + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(parameter_name);
    auto connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    connection->controller = _controller;

    auto cb = _osc->add_method(osc_path.c_str(), "f", osc::OscMethodType::SEND_PARAMETER_CHANGE_EVENT, connection);
    connection->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);

    return connection;
}

OscConnection* OSCFrontend::_connect_to_property(const std::string& processor_name,
                                                 const std::string& property_name,
                                                 ObjectId processor_id,
                                                 ObjectId property_id)
{
    assert(_osc_initialized);
    if (_osc_initialized == false)
    {
        return nullptr;
    }

    std::string osc_path = "/property/" + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(property_name);
    auto connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = property_id;
    connection->instance = this;
    connection->controller = _controller;

    auto cb = _osc->add_method(osc_path.c_str(), "s", osc::OscMethodType::SEND_PROPERTY_CHANGE_EVENT, connection);
    connection->callback = cb;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    SUSHI_LOG_DEBUG("Added osc callback {}", osc_path);

    return connection;
}

void OSCFrontend::_connect_from_property(const std::string& processor_name,
                                         const std::string& property_name,
                                         ObjectId processor_id,
                                         ObjectId property_id)
{
    std::string id_string = "/property/" + osc::make_safe_path(processor_name) + "/" + osc::make_safe_path(property_name);

    _outgoing_connections[processor_id][property_id] = id_string;

    SUSHI_LOG_DEBUG("Added osc output from property {}/{}", processor_name, property_name);
}

void OSCFrontend::_connect_to_parameters_and_properties(const Processor* processor)
{
    auto parameters = processor->all_parameters();
    for (auto& param : parameters)
    {
        auto type = param->type();
        if (type == ParameterType::FLOAT || type == ParameterType::INT || type == ParameterType::BOOL)
        {
            _connect_to_parameter(processor->name(), param->name(), processor->id(), param->id());
        }
        else if (type == ParameterType::STRING)
        {
            _connect_to_property(processor->name(), param->name(), processor->id(), param->id());
            // TODO - for now property outputs are always on.
            _connect_from_property(processor->name(), param->name(), processor->id(), param->id());
        }
    }
}

void OSCFrontend::_setup_engine_control()
{
    _set_tempo_cp = _osc->add_method("/engine/set_tempo", "f", osc::OscMethodType::SET_TEMPO, this->_controller);
    _set_time_signature_cb = _osc->add_method("/engine/set_time_signature", "ii", osc::OscMethodType::SET_TIME_SIGNATURE, this->_controller);
    _set_playing_mode_cb = _osc->add_method("/engine/set_playing_mode", "s", osc::OscMethodType::SET_PLAYING_MODE, this->_controller);
    _set_sync_mode_cb = _osc->add_method("/engine/set_sync_mode", "s", osc::OscMethodType::SET_TEMPO_SYNC_MODE, this->_controller);
    _set_timing_statistics_enabled_cb = _osc->add_method("/engine/set_timing_statistics_enabled", "i",
                                                         osc::OscMethodType::SET_TIMING_STATISTICS_ENABLED, this->_controller);
    _reset_timing_statistics_s_cb = _osc->add_method("/engine/reset_timing_statistics", "s",
                                                     osc::OscMethodType::RESET_TIMING_STATISTICS, this->_controller);
    _reset_timing_statistics_ss_cb = _osc->add_method("/engine/reset_timing_statistics", "ss",
                                                      osc::OscMethodType::RESET_TIMING_STATISTICS, this->_controller);
}

void OSCFrontend::_completion_callback(Event* event, int return_status)
{
    SUSHI_LOG_DEBUG("EngineEvent {} completed with status {}({})", event->id(), return_status == 0 ? "ok" : "failure", return_status);
}

void OSCFrontend::_start_server()
{
    assert(_osc_initialized);

    _running.store(true);

    _osc->run();
}

void OSCFrontend::_stop_server()
{
    assert(_osc_initialized);

    _running.store(false);
    _osc->stop();
}

bool OSCFrontend::_remove_processor_connections(ObjectId processor_id)
{
    assert(_osc_initialized);

    int count = 0;
    for (const auto& c : _connections)
    {
        if (c->processor == processor_id)
        {
            _osc->delete_method(c->callback);

            count++;
        }
    }
    _connections.erase(std::remove_if(_connections.begin(),
                                      _connections.end(),
                                      [&](const auto& c) { return c->processor == processor_id; }),
                       _connections.end());

    count += _outgoing_connections.erase(static_cast<ObjectId>(processor_id));

    SUSHI_LOG_ERROR_IF(count == 0, "Failed to remove any connections for processor {}", processor_id)
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
            _osc->send(param_node->second.c_str(), event->normalized_value());
            SUSHI_LOG_DEBUG("Sending parameter change from processor: {}, parameter: {}, value: {}",
                            event->processor_id(), event->parameter_id(), event->normalized_value());
        }
    }
}

void OSCFrontend::_handle_property_change_notification(const PropertyChangeNotificationEvent* event)
{
    const auto& node = _outgoing_connections.find(event->processor_id());
    if (node != _outgoing_connections.end())
    {
        const auto& param_node = node->second.find(event->property_id());
        if (param_node != node->second.end())
        {
            _osc->send(param_node->second.c_str(), event->value());
            SUSHI_LOG_DEBUG("Sending property change from processor: {}, property: {}, value: {}",
                            event->processor_id(), event->property_id(), event->value());
        }
    }
}

void OSCFrontend::_handle_clipping_notification(const ClippingNotificationEvent* event)
{
    if (event->channel_type() == ClippingNotificationEvent::ClipChannelType::INPUT)
    {
        _osc->send("/engine/input_clip_notification", event->channel());
    }
    else if (event->channel_type() == ClippingNotificationEvent::ClipChannelType::OUTPUT)
    {
        _osc->send("/engine/output_clip_notification", event->channel());
    }
}

void OSCFrontend::_handle_audio_graph_notification(const AudioGraphNotificationEvent* event)
{
    switch(event->action())
    {
        case AudioGraphNotificationEvent::Action::PROCESSOR_CREATED:
        {
            SUSHI_LOG_DEBUG("Received a PROCESSOR_CREATED notification for processor {}", event->processor());
            auto processor = _processor_container->processor(event->processor());
            if (processor)
            {
                _connect_to_bypass_state(processor.get());
                _connect_to_program_change(processor.get());
                _connect_to_parameters_and_properties(processor.get());
                if(_connect_from_all_parameters && _skip_outputs.count(processor->id()) == false)
                {
                    connect_from_processor_parameters(processor->name());
                    SUSHI_LOG_INFO("Connected OSC callbacks to processor {}", processor->id());
                }
                _skip_outputs.erase(processor->id());
            }
            SUSHI_LOG_ERROR_IF(!processor , "Processor {} not found", event->processor())
            break;
        }

        case AudioGraphNotificationEvent::Action::TRACK_CREATED:
        {
            SUSHI_LOG_DEBUG("Received a TRACK_ADDED notification for track {}", event->track());
            auto track = _processor_container->track(event->track());
            if (track)
            {
                _connect_kb_to_track(track.get());
                _connect_to_bypass_state(track.get());
                _connect_to_parameters_and_properties(track.get());
                if(_connect_from_all_parameters && _skip_outputs.count(track->id()) == false)
                {
                    connect_from_processor_parameters(track->name());
                    SUSHI_LOG_INFO("Connected OSC callbacks to track {}", track->name());
                }
                _skip_outputs.erase(track->id());
            }
            SUSHI_LOG_ERROR_IF(!track, "Track {} not found", event->track())
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

} // namespace sushi
