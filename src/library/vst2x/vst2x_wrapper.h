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
 * @brief Wrapper for VST 2.x plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST2X_PLUGIN_H
#define SUSHI_VST2X_PLUGIN_H

#include <map>

#include "library/processor.h"
#include "library/constants.h"
#include "vst2x_plugin_loader.h"
#include "vst2x_midi_event_fifo.h"
#include "engine/base_event_dispatcher.h"

namespace sushi {
namespace vst2 {

/* Should match the maximum reasonable number of channels of a vst */
constexpr int VST_WRAPPER_MAX_N_CHANNELS = MAX_TRACK_CHANNELS;
constexpr int VST_WRAPPER_MIDI_EVENT_QUEUE_SIZE = 256;

/**
 * @brief internal wrapper class for loading VST plugins and make them accesible as Processor to the Engine.
 */

class Vst2xWrapper : public Processor
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Vst2xWrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Vst2xWrapper(HostControl host_control, const std::string &vst_plugin_path) :
            Processor(host_control),
            _sample_rate{0},
            _process_inputs{},
            _process_outputs{},
            _can_do_soft_bypass{false},
            _plugin_path{vst_plugin_path},
            _library_handle{nullptr},
            _plugin_handle{nullptr}
    {
        _max_input_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = VST_WRAPPER_MAX_N_CHANNELS;
    }

    ~Vst2xWrapper() override;

    /* Inherited from Processor */
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    bool bypassed() const override {return _bypass_manager.bypassed();}

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_in_domain(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    bool supports_programs() const override {return _number_of_programs > 0;}

    int program_count() const override {return _number_of_programs;}

    int current_program() const override;

    std::string current_program_name() const override;

    std::pair<ProcessorReturnCode, std::string> program_name(int program) const override;

    std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const override;

    ProcessorReturnCode set_program(int program) override;

    ProcessorReturnCode set_state(ProcessorState* state, bool realtime_running) override;

    ProcessorState save_state() const override;

    PluginInfo info() const override;

    /**
     * @brief Get the vst time information
     * @return A populated VstTimeInfo struct
     */
    VstTimeInfo* time_info();

private:
    friend VstIntPtr VSTCALLBACK host_callback(AEffect* effect,
                                               VstInt32 opcode, VstInt32 index,
                                               VstIntPtr value, void* ptr, float opt);

    /**
     * @brief Notify the host of a parameter change from inside the plugin
     *        This must be called from the realtime thread
     * @param parameter_index The index of the parameter that has changed
     * @param value The new value of the parameter
     */
    void notify_parameter_change_rt(VstInt32 parameter_index, float value);

    /**
      * @brief Notify the host of a parameter change from inside the plugin
      *        This must be called from a non-rt thread and not from the
      *        audio thread
      * @param parameter_index The index of the parameter that has changed
      * @param value The new value of the parameter
      */
    void notify_parameter_change(VstInt32 parameter_index, float value);

    /**
     * @brief Output a vst midi event from the plugin.
     * @param event Pointer to a VstEvent struct
     */
    void output_vst_event(const VstEvent* event);
    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Commodity function to access VsT internals
     */
    int _vst_dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) const
    {
        return static_cast<int>(_plugin_handle->dispatcher(_plugin_handle, opcode, index, value, ptr, opt));
    }

    /**
     * @brief Iterate over VsT parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    bool _update_speaker_arrangements(int inputs, int outputs);

    void _map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer);

    void _set_bypass_rt(bool bypassed);

    void _set_state_rt(RtState* state);

    float _sample_rate;
    /** Wrappers for preparing data to pass to processReplacing */
    float* _process_inputs[VST_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[VST_WRAPPER_MAX_N_CHANNELS];
    ChunkSampleBuffer _dummy_input{1};
    ChunkSampleBuffer _dummy_output{1};
    Vst2xMidiEventFIFO<VST_WRAPPER_MIDI_EVENT_QUEUE_SIZE> _vst_midi_events_fifo;
    bool _can_do_soft_bypass;
    bool _has_binary_programs{false};
    int _number_of_programs{0};

    BypassManager _bypass_manager{_bypassed};

    std::string _plugin_path;
    LibraryHandle _library_handle;
    AEffect* _plugin_handle;

    VstTimeInfo _time_info;
};

VstSpeakerArrangementType arrangement_from_channels(int channels);

} // end namespace vst2
} // end namespace sushi

#endif //SUSHI_VST2X_PLUGIN_H
