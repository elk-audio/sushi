/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST2X_PLUGIN_H
#define SUSHI_VST2X_PLUGIN_H

#include "library/processor.h"
#include "library/vst2x_plugin_loader.h"
#include "library/vst2x_midi_event_fifo.h"

#include <map>

namespace sushi {
namespace vst2 {

// TODO:
//      For now, put a fixed number to avoid dynamic relocation of _process_[in|out]puts,
//      and because we have to rework that part anyway (it's not flexible in InternalPlugin
//      as well
constexpr int VST_WRAPPER_MAX_N_CHANNELS = 2;
constexpr int VST_WRAPPER_MIDI_EVENT_QUEUE_SIZE = 256;

/**
 * @brief internal wrapper class for loading VST plugins and make them accesible as Processor to the Engine.
 */

class Vst2xWrapper : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(Vst2xWrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */

    Vst2xWrapper(const std::string &vst_plugin_path) :
            _sample_rate{0},
            _process_inputs{},
            _process_outputs{},
            _plugin_path{vst_plugin_path},
            _library_handle{nullptr},
            _plugin_handle{nullptr}
    {
        _max_input_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = VST_WRAPPER_MAX_N_CHANNELS;
    }

    virtual ~Vst2xWrapper()
    {
        _cleanup();
    }

    /* Inherited from Processor */
    ProcessorReturnCode init(const int sample_rate) override;

    std::pair<ProcessorReturnCode, ObjectId> parameter_id_from_name(const std::string& parameter_name) override;

    void process_event(Event event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    void set_enabled(bool enabled) override;

private:
    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Commodity function to access VsT internals
     */
    void _vst_dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
    {
        _plugin_handle->dispatcher(_plugin_handle, opcode, index, value, ptr, opt);
    }

    /**
     * @brief Iterate over VsT parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    int _sample_rate;
    /** Wrappers for preparing data to pass to processReplacing */
    float* _process_inputs[VST_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[VST_WRAPPER_MAX_N_CHANNELS];
    Vst2xMidiEventFIFO<VST_WRAPPER_MIDI_EVENT_QUEUE_SIZE> _vst_midi_events_fifo;

    std::string _plugin_path;
    LibraryHandle _library_handle;
    AEffect *_plugin_handle;

    std::map<std::string, ObjectId> _param_names_to_id;
};

} // end namespace vst2
} // end namespace sushi

#endif //SUSHI_VST2X_PLUGIN_H
