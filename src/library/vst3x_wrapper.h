/**
 * @Brief Wrapper for VST 3.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST3X_WRAPPER_H
#define SUSHI_VST3X_WRAPPER_H

#ifdef SUSHI_BUILD_WITH_VST3

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

constexpr int VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE = 256;

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
    Vst3xWrapper(HostControl host_control, const std::string& vst_plugin_path, const std::string& plugin_name) :
            Processor(host_control),
            _loader{vst_plugin_path, plugin_name}
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
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

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

    /**
     * @brief Read output events from the plugin, convert to internal events
     *        and forward to next plugin.
     * @param data A ProcessData container
     */
    void _forward_events(Steinberg::Vst::ProcessData& data);

    void _fill_processing_context();

    inline void _add_parameter_change(Steinberg::Vst::ParamID id, float value, int sample_offset);

    struct SpecialParameter
    {
        bool supported{false};
        Steinberg::Vst::ParamID id{0};
    };

    float _sample_rate;
    PluginLoader _loader;
    PluginInstance _instance;

    Steinberg::Vst::EventList _in_event_list{VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE};
    Steinberg::Vst::EventList _out_event_list{VST_WRAPPER_NOTE_EVENT_QUEUE_SIZE};
    Steinberg::Vst::ParameterChanges _in_parameter_changes;
    Steinberg::Vst::ParameterChanges _out_parameter_changes;

    SushiProcessData _process_data{&_in_event_list,
                                   &_out_event_list,
                                   &_in_parameter_changes,
                                   &_out_parameter_changes};

    SpecialParameter _bypass_parameter;
    SpecialParameter _pitch_bend_parameter;
    SpecialParameter _mod_wheel_parameter;
    SpecialParameter _aftertouch_parameter;
};

Steinberg::Vst::SpeakerArrangement speaker_arr_from_channels(int channels);

} // end namespace vst3
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_VST3
#ifndef SUSHI_BUILD_WITH_VST3

#include "library/processor.h"

namespace sushi {
namespace vst3 {
/* If Vst 3 support is disabled in the build, the wrapper is replaced with this
   minimal dummy processor whose purpose is to log an error message if a user
   tries to load a Vst 3 plugin */
class Vst3xWrapper : public Processor
{
public:
    Vst3xWrapper(HostControl host_control, const std::string& /* vst_plugin_path */, const std::string& /* plugin_name */) :
         Processor(host_control) {}
    ProcessorReturnCode init(float sample_rate) override;
    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer & /*in*/, ChunkSampleBuffer & /*out*/) override {}
};

}// end namespace vst3
}// end namespace sushi
#endif
#endif //SUSHI_VST3X_WRAPPER_H
