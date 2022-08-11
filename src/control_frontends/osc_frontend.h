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
 *
 * Starts a thread listening for OSC commands at the given port
 * (configurable with proper command sent with apply_command.
 *
 */

#ifndef SUSHI_OSC_FRONTEND_H_H
#define SUSHI_OSC_FRONTEND_H_H

#include <vector>
#include <map>

#include "control_interface.h"
#include "base_control_frontend.h"

namespace sushi {

namespace osc
{
class BaseOscMessenger;
}

namespace control_frontend {

class OSCFrontend;

class OscState
{
public:

    bool auto_enable_outputs() const;

    void set_auto_enable_outputs(bool value);

    const std::vector<std::pair<std::string, std::vector<ObjectId>>>& enabled_outputs() const;

    void add_enabled_outputs(const std::string& processor_name,
                             const std::vector<ObjectId>& enabled_parameters);

    void add_enabled_outputs(std::string&& processor_name,
                             std::vector<ObjectId>&& enabled_parameters);

private:
    bool _auto_enable_outputs;
    std::vector<std::pair<std::string, std::vector<ObjectId>>> _enabled_outputs;
};

struct OscConnection
{
    ObjectId           processor;
    ObjectId           parameter;
    OSCFrontend*       instance;
    ext::SushiControl* controller;

    void* callback;
};

class OSCFrontend : public BaseControlFrontend
{
public:
    OSCFrontend(engine::BaseEngine* engine,
                ext::SushiControl* controller, osc::BaseOscMessenger* osc_interface);

    ~OSCFrontend();

    ControlFrontendStatus init() override;

    /**
     * @brief Output changes from the given parameter of the given
     *        processor to osc messages. The output will be on the form:
     *        "/parameter/processor_name/parameter_name,f(value)"
     * @param processor_name Name of the processor
     * @param parameter_name Name of the processor's parameter
     * @return Bool of whether connection succeeded.
     */
    bool connect_from_parameter(const std::string& processor_name,
                                const std::string& parameter_name);

    /**
     * @brief Stops the broadcasting of OSC messages reflecting changes of a parameter.
     * @param processor_name Name of the processor
     * @param parameter_name Name of the processor's parameter
     * @return Bool of whether disconnection succeeded.
     */
    bool disconnect_from_parameter(const std::string& processor_name,
                                   const std::string& parameter_name);


    /**
     * @brief Enable OSC broadcasting of all parameters from a given processor.
     * @param processor_name The name of the processor to connect.
     * @param processor_id The id of the processor to connect.
     * @return Bool of whether connection succeeded.
     */
    bool connect_from_processor_parameters(const std::string& processor_name);

    /**
     * @brief Register OSC callbacks for all parameters of all plugins.
     */
    void connect_from_all_parameters();

    /**
     * @brief Deregister OSC callbacks for all parameters of all plugins.
     */
    void disconnect_from_all_parameters();

    /**
     * @return Returns all OSC Address Patterns that are currently enabled to output state changes.
     */
    std::vector<std::string> get_enabled_parameter_outputs();

    void run() override {_start_server();}

    void stop() override {_stop_server();}

    /* Inherited from EventPoster */
    int process(Event* event) override;

    int poster_id() override {return EventPosterId::OSC_FRONTEND;}

    std::string send_ip() const;

    int send_port() const;

    int receive_port() const;

    bool get_connect_from_all_parameters() {return _connect_from_all_parameters;}

    void set_connect_from_all_parameters(bool connect) {_connect_from_all_parameters = connect;}

    OscState save_state() const;

    void set_state(const OscState& state);

private:
    /**
     * @brief Connect to control all parameters from a given processor.
     * @param processor The name of the processor to connect.
     * @param processor_id The id of the processor to connect.
     * @return Bool of whether connection succeeded.
     */
    void _connect_to_parameters_and_properties(const Processor* processor);

    /**
     * @brief Connect keyboard messages to a given track.
     *        The target osc path will be:
     *        "/keyboard_event/track_name,sif(note_on/note_off, note_value, velocity)"
     * @param processor The track to send to
     * @return An OscConnection pointer, if one has been created - otherwise nullptr.
     */
    OscConnection* _connect_kb_to_track(const Processor* processor);

    /**
     * @brief Connect osc to the bypass state of a given processor.
     *        The resulting osc path will be:
     *        "/bypass/processor_name,i(enabled == 1, disabled == 0)"
     *
     * @param processor_name Name of the processor
     * @return An OscConnection pointer, if one has been created - otherwise nullptr.
     */
    OscConnection* _connect_to_bypass_state(const Processor* processor);

    /**
     * @brief Connect program change messages to a specific processor.
     *        The resulting osc path will be;
     *        "/program/processor i (program_id)"
     * @param processor Name of the processor
     * @return An OscConnection pointer, if one has been created - otherwise nullptr.
     */
    OscConnection* _connect_to_program_change(const Processor* processor);

    OscConnection* _connect_to_parameter(const std::string& processor_name,
                                         const std::string& parameter_name,
                                         ObjectId processor_id,
                                         ObjectId parameter_id);

    OscConnection* _connect_to_property(const std::string& processor_name,
                                        const std::string& property_name,
                                        ObjectId processor_id,
                                        ObjectId property_id);

    void _connect_from_parameter(const std::string& processor_name,
                                 const std::string& parameter_name,
                                 ObjectId processor_id,
                                 ObjectId parameter_id);

    void _connect_from_property(const std::string& processor_name,
                                const std::string& property_name,
                                ObjectId processor_id,
                                ObjectId property_id);

    void _completion_callback(Event* event, int return_status) override;

    void _start_server();

    void _stop_server();

    void _setup_engine_control();

    bool _remove_processor_connections(ObjectId processor_id);

    std::pair<OscConnection*, std::string> _create_processor_connection(const std::string& processor_name,
                                                                        ObjectId processor_id,
                                                                        const std::string& osc_path_prefix);

    void _handle_param_change_notification(const ParameterChangeNotificationEvent* event);

    void _handle_property_change_notification(const PropertyChangeNotificationEvent* event);

    void _handle_engine_notification(const EngineNotificationEvent* event);

    void _handle_audio_graph_notification(const AudioGraphNotificationEvent* event);

    void _handle_clipping_notification(const ClippingNotificationEvent* event);

    bool _connect_from_all_parameters {false};

    std::atomic_bool _osc_initialized {false};

    std::atomic_bool _running {false};

    sushi::ext::SushiControl* _controller {nullptr};
    sushi::ext::AudioGraphController* _graph_controller {nullptr};
    sushi::ext::ParameterController*  _param_controller {nullptr};

    const engine::BaseProcessorContainer* _processor_container;

    std::unique_ptr<osc::BaseOscMessenger> _osc {nullptr};

    std::vector<std::unique_ptr<OscConnection>> _connections;

    std::map<ObjectId, std::map<ObjectId, std::string>> _outgoing_connections;

    std::map<ObjectId, bool> _skip_outputs;

    void* _set_tempo_cp {nullptr};
    void* _set_time_signature_cb {nullptr};
    void* _set_playing_mode_cb {nullptr};
    void* _set_sync_mode_cb {nullptr};
    void* _set_timing_statistics_enabled_cb {nullptr};
    void* _reset_timing_statistics_s_cb {nullptr};
    void* _reset_timing_statistics_ss_cb {nullptr};
};

} // namespace control_frontend
} // namespace sushi

#endif //SUSHI_OSC_FRONTEND_H_H
