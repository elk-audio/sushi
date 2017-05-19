/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST2X_PLUGIN_H
#define SUSHI_VST2X_PLUGIN_H

#include "library/plugin_parameters.h"
#include "library/processor.h"
#include "library/vst2x_plugin_loader.h"

#include <algorithm>
#include <map>

namespace sushi {
namespace vst2 {

// TODO:
//      For now, put a fixed number to avoid dynamic relocation of _process_[in|out]puts,
//      and because we have to rework that part anyway (it's not flexible in InternalPlugin
//      as well

constexpr int VST_WRAPPER_MAX_N_CHANNELS = 2;

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

    void process_event(Event /* event */) override;

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

    // /**
    // * @brief Return the parameter with the given unique id
    // */
    // BaseStompBoxParameter *get_parameter(const std::string &id)
    // {
    //     auto parameter = _parameters.find(id);
    //     if (parameter == _parameters.end())
    //     {
    //         return nullptr;
    //     }
    //     return parameter->second.get();
    // }

    // void register_parameter(BaseStompBoxParameter *parameter)
    // {
    //     _parameters.insert(std::pair < std::string, std::unique_ptr < BaseStompBoxParameter >> (parameter->id(),
    //             std::unique_ptr < BaseStompBoxParameter > (parameter)));
    // }

    int _sample_rate;
    /** Wrappers for preparing data to pass to processReplacing */
    float* _process_inputs[VST_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[VST_WRAPPER_MAX_N_CHANNELS];

    std::string _plugin_path;
    LibraryHandle _library_handle;
    AEffect *_plugin_handle;

    std::map<std::string, std::unique_ptr<BaseStompBoxParameter>> _parameters;
};

} // end namespace vst2
} // end namespace sushi

#endif //SUSHI_VST2X_PLUGIN_H
