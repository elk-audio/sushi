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
 * @brief Wrapper for VST 3.x plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST3X_WRAPPER_H
#define SUSHI_VST3X_WRAPPER_H

#include <map>
#include <utility>

#include "pluginterfaces/base/ipluginbase.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"

#include "vst3x_host_app.h"
#include "library/processor.h"
#include "vst3x_utils.h"

namespace sushi {
namespace vst3 {

constexpr int VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE = 256;

/**
 * @brief internal wrapper class for loading VST plugins and make them accessible as Processor to the Engine.
 */
class Vst3xWrapper : public Processor
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Vst3xWrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Vst3xWrapper(HostControl host_control,
                 const std::string& vst_plugin_path,
                 const std::string& plugin_name,
                 SushiHostApplication* host_app) :
            Processor(host_control),
            _plugin_load_name(plugin_name),
            _plugin_load_path(vst_plugin_path),
            _instance(host_app),
            _component_handler(this, &_host_control)
    {
        _max_input_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _enabled = false;
    }

    ~Vst3xWrapper() override;

    /**
     * @brief Entry point for parameter changes from the plugin editor.
     * @param param_id Id of the parameter to change
     * @param value New value for parameter
     */
    void set_parameter_change(ObjectId param_id, float value);

    /* Inherited from Processor */
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    bool bypassed() const override;

    const ParameterDescriptor* parameter_from_id(ObjectId id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_in_domain(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    bool supports_programs() const override {return _supports_programs;}

    int program_count() const override {return _program_count;}

    int current_program() const override;

    std::string current_program_name() const override;

    std::pair<ProcessorReturnCode, std::string> program_name(int program) const override;

    std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const override;

    ProcessorReturnCode set_program(int program) override;

    ProcessorReturnCode set_state(ProcessorState* state, bool realtime_running) override;

    ProcessorState save_state() const override;

    PluginInfo info() const override;

    static void program_change_callback(void* arg, Event* event, int status)
    {
        reinterpret_cast<Vst3xWrapper*>(arg)->_program_change_callback(event, status);
    }

    static int parameter_update_callback(void* data, EventId id)
    {
        return reinterpret_cast<Vst3xWrapper*>(data)->_parameter_update_callback(id);
    }

private:
    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Iterate over VsT parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    bool _setup_audio_buses();

    bool _setup_event_buses();

    bool _setup_channels();

    bool _setup_processing();

    bool _setup_internal_program_handling();

    bool _setup_file_program_handling();

    /**
     * @brief Read output events from the plugin, convert to internal events
     *        and forward to next plugin.
     * @param data A ProcessData container
     */
    void _forward_events(Steinberg::Vst::ProcessData& data);

    void _forward_params(Steinberg::Vst::ProcessData& data);

    void _fill_processing_context();

    inline void _add_parameter_change(Steinberg::Vst::ParamID id, float value, int sample_offset);

    bool _sync_processor_to_controller();

    void _program_change_callback(Event* event, int status);

    int _parameter_update_callback(EventId id);

    void _set_program_state(int program_id, RtState* rt_state, bool realtime_running);

    void _set_bypass_state(bool bypassed, RtState* rt_state, bool realtime_running);

    void _set_binary_state(std::vector<std::byte>& state);

    void _set_state_rt(Vst3xRtState* state);

    struct SpecialParameter
    {
        bool supported{false};
        Steinberg::Vst::ParamID id{0};
    };

    struct ParameterUpdate
    {
        Steinberg::Vst::ParamID id;
        float value;
    };

    float _sample_rate;
    bool  _supports_programs{false};
    bool  _internal_programs{false};
    bool  _file_based_programs{false};
    int _main_program_list_id;
    int _program_count{0};
    int _current_program{0};

    bool _notify_parameter_change{false};

    BypassManager _bypass_manager{_bypassed};

    std::vector<std::string> _program_files;

    std::string _plugin_load_name;
    std::string _plugin_load_path;
    PluginInstance _instance;
    ComponentHandler _component_handler;

    Steinberg::Vst::EventList _in_event_list{VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE};
    Steinberg::Vst::EventList _out_event_list{VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE};
    Steinberg::Vst::ParameterChanges _in_parameter_changes;
    Steinberg::Vst::ParameterChanges _out_parameter_changes;
    Vst3xRtState* _state_parameter_changes{nullptr};

    SushiProcessData _process_data{&_in_event_list,
                                   &_out_event_list,
                                   &_in_parameter_changes,
                                   &_out_parameter_changes};

    SpecialParameter _bypass_parameter;
    SpecialParameter _program_change_parameter;
    SpecialParameter _pitch_bend_parameter;
    SpecialParameter _mod_wheel_parameter;
    SpecialParameter _aftertouch_parameter;

    memory_relaxed_aquire_release::CircularFifo<ParameterUpdate, 100> _parameter_update_queue;
    std::map<Steinberg::Vst::ParamID, const ParameterDescriptor*> _parameters_by_vst3_id;
    friend class ComponentHandler;
};

Steinberg::Vst::SpeakerArrangement speaker_arr_from_channels(int channels);

} // end namespace vst3
} // end namespace sushi

#endif //SUSHI_VST3X_WRAPPER_H
