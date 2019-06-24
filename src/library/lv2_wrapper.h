/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_LV2_PLUGIN_H
#define SUSHI_LV2_PLUGIN_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>

#include "library/processor.h"
#include "library/lv2_plugin_loader.h"
//#include "library/lv2_midi_event_fifo.h"
#include "../engine/base_event_dispatcher.h"

namespace sushi {
namespace lv2 {

/* Should match the maximum reasonable number of channels of a vst */
constexpr int LV2_WRAPPER_MAX_N_CHANNELS = 8;
constexpr int LV2_WRAPPER_MIDI_EVENT_QUEUE_SIZE = 256;

/**
 * @brief internal wrapper class for loading VST plugins and make them accesible as Processor to the Engine.
 */

class Lv2Wrapper : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(Lv2Wrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Lv2Wrapper(HostControl host_control, const std::string &vst_plugin_path) :
            Processor(host_control),
            _sample_rate{0},
            _process_inputs{},
            _process_outputs{},
            _can_do_soft_bypass{false},
            _double_mono_input{false},
            _plugin_path{vst_plugin_path},
            _library_handle{nullptr},
            _plugin_handle{nullptr}
    {
        _max_input_channels = LV2_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = LV2_WRAPPER_MAX_N_CHANNELS;
    }

    virtual ~Lv2Wrapper()
    {
        _cleanup();
    }

    /* Inherited from Processor */
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_normalised(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    bool supports_programs() const override {return _number_of_programs > 0;}

    int program_count() const override {return _number_of_programs;}

    int current_program() const override;

    std::string current_program_name() const override;

    std::pair<ProcessorReturnCode, std::string> program_name(int program) const override;

    std::pair<ProcessorReturnCode, std::vector<std::string>> all_program_names() const override;

    ProcessorReturnCode set_program(int program) override;

    /**
     * @brief Notify the host of a parameter change from inside the plugin
     *        This must be called from the realtime thread
     * @param parameter_index The index of the parameter that has changed
     * @param value The new value of the parameter
     *//*
    void notify_parameter_change_rt(VstInt32 parameter_index, float value);

    *//**
      * @brief Notify the host of a parameter change from inside the plugin
      *        This must be called from a non-rt thread and not from the
      *        audio thread
      * @param parameter_index The index of the parameter that has changed
      * @param value The new value of the parameter
      *//*
    void notify_parameter_change(VstInt32 parameter_index, float value);*/

    /**
     * @brief Get the vst time information
     * @return A populated VstTimeInfo struct
     */
//    VstTimeInfo* time_info();

private:
    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Commodity function to access VsT internals
     */
    /*int _vst_dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) const
    {
        //return static_cast<int>(_plugin_handle->dispatcher(_plugin_handle, opcode, index, value, ptr, opt));
        return 0;
    }*/

    /**
     * @brief Iterate over VsT parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    bool _update_speaker_arrangements(int inputs, int outputs);

    /**
     * @brief For plugins that support stereo I/0 and not mono through SetSpeakerArrangements,
     *        we can provide the plugin with a dual mono input/output instead.
     *        Calling this sets up possible dual mono mode.
     * @param speaker_arr_status True if the plugin supports the given speaker arrangement.
     */
    void _update_mono_mode(bool speaker_arr_status);

    void _map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer);

    float _sample_rate;
    /** Wrappers for preparing data to pass to processReplacing */
    float* _process_inputs[LV2_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[LV2_WRAPPER_MAX_N_CHANNELS];
    ChunkSampleBuffer _dummy_input{1};
    ChunkSampleBuffer _dummy_output{1};
//    Lv2MidiEventFIFO<LV2_WRAPPER_MIDI_EVENT_QUEUE_SIZE> _vst_midi_events_fifo;
    bool _can_do_soft_bypass;
    bool _double_mono_input;
    int _number_of_programs{0};

    std::string _plugin_path;
    LibraryHandle _library_handle;
    //AEffect*_plugin_handle;
    void*_plugin_handle;


//    VstTimeInfo _time_info;
};

//VstSpeakerArrangementType arrangement_from_channels(int channels);

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

#include "library/processor.h"

namespace sushi {
namespace lv2 {
/* If LV2 support is disabled in the build, the wrapper is replaced with this
   minimal dummy processor whose purpose is to log an error message if a user
   tries to load an LV2 plugin */
class Lv2Wrapper : public Processor
{
public:
    Lv2Wrapper(HostControl host_control, const std::string& /* vst_plugin_path */, const std::string& /* plugin_name */) :
        Processor(host_control) {}
    ProcessorReturnCode init(float sample_rate) override;
    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer & /*in*/, ChunkSampleBuffer & /*out*/) override {}
};

}// end namespace lv2
}// end namespace sushi
#endif

#endif //SUSHI_LV2_PLUGIN_H