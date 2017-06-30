/**
 * @Brief Wrapper for VST 3.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST3X_WRAPPER_H
#define SUSHI_VST3X_WRAPPER_H

#include <map>
#include <utility>

#include "pluginterfaces/base/ipluginbase.h"
#define DEVELOPMENT
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#undef DEVELOPMENT

#include "library/vst3x_host_app.h"
#include "library/processor.h"
#include "library/vst3x_utils.h"

namespace sushi {
namespace vst3 {

constexpr int VST_WRAPPER_MIDI_EVENT_QUEUE_SIZE = 256;

/**
 * @brief internal wrapper class for loading VST plugins and make them accesible as Processor to the Engine.
 */
class Vst3xWrapper : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(Vst3xWrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Vst3xWrapper(const std::string& vst_plugin_path, const std::string& plugin_name) : _loader{vst_plugin_path,
                                                                                               plugin_name}
    {
        _max_input_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _max_output_channels = VST_WRAPPER_MAX_N_CHANNELS;
        _enabled = false;
    }

    virtual ~Vst3xWrapper()
    {
        _cleanup();
    }

    /* Inherited from Processor */
    ProcessorReturnCode init(const int sample_rate) override;

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
     * @brief Iterate over VsT parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    bool _setup_audio_busses();

    bool _setup_event_busses();

    bool _setup_channels();

    bool _setup_processing();


    int _sample_rate;
    /** Wrappers for preparing data to pass to processReplacing */

    PluginLoader _loader;

    PluginInstance _instance;

    Steinberg::Vst::EventList _in_event_list;
    Steinberg::Vst::EventList _out_event_list;
    Steinberg::Vst::ParameterChanges _in_parameter_changes;
    Steinberg::Vst::ParameterChanges _out_parameter_changes;

    SushiProcessData _process_data{&_in_event_list,
                                   &_out_event_list,
                                   &_in_parameter_changes,
                                   &_out_parameter_changes};

};

} // end namespace vst3
} // end namespace sushi

#endif //SUSHI_VST3X_WRAPPER_H
