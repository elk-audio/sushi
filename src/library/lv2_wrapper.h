/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_LV2_PLUGIN_H
#define SUSHI_LV2_PLUGIN_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>

#include "processor.h"
#include "lv2_plugin_loader.h"

#include "lv2_evbuf.h"
#include "../engine/base_event_dispatcher.h"

#include "library/rt_event_fifo.h"

#include "midi_encoder.h"
#include "midi_decoder.h"

namespace sushi {
namespace lv2 {

/* Should match the maximum reasonable number of channels of a vst */
constexpr int LV2_WRAPPER_MAX_N_CHANNELS = 8;

/**
 * @brief internal wrapper class for loading VST plugins and make them accessible as Processor to the Engine.
 */

class Lv2Wrapper : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(Lv2Wrapper)
    /**
     * @brief Create a new Processor that wraps the plugin found in the given path.
     */
    Lv2Wrapper(HostControl host_control, const std::string& lv2_plugin_uri) :
            Processor(host_control),
            _sample_rate{0},
            _process_inputs{},
            _process_outputs{},
            _double_mono_input{false},
            _plugin_path{lv2_plugin_uri}
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

private:
    /**
    * @brief Allocates LV2 port buffers (only necessary for MIDI)
    */
    void _allocate_port_buffers(LV2Model *model);

    void _create_ports(const LilvPlugin *plugin);
    void _create_port(const LilvPlugin *plugin, int port_index, float default_value);

    /**
     * @brief Tell the plugin that we're done with it and release all resources
     * we allocated during initialization.
     */
    void _cleanup();

    /**
     * @brief Iterate over LV2 parameters and register internal FloatParameterDescriptor
     *        for each one of them.
     * @return True if all parameters were registered properly.
     */
    bool _register_parameters();

    /**
     * @brief For plugins that support stereo I/0 and not mono through SetSpeakerArrangements,
     *        we can provide the plugin with a dual mono input/output instead.
     *        Calling this sets up possible dual mono mode.
     * @param speaker_arr_status True if the plugin supports the given speaker arrangement.
     */
    void _update_mono_mode(bool speaker_arr_status);

    bool _check_for_required_features(const LilvPlugin* plugin);
    void _fetch_plugin_name_and_label();

    void _map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer);

    void _deliver_inputs_to_plugin();
    void _deliver_outputs_from_plugin(bool send_ui_updates);

    void _convert_event_to_midi_buffer(RtEvent& event);
    void _flush_event_queue();
    void _process_midi_input_for_current_port();
    void _process_midi_output_for_current_port();

    void _notify_parameter_change_rt(int parameter_index, float value);
    void _notify_parameter_change(int parameter_index, float value);

    void jalv_set_control(const ControlID* control, uint32_t size, LV2_URID type, const void* body);
    void _create_controls(LV2Model *model, bool writable);

//  void jalv_ui_instantiate(LV2Model* model, const char* native_ui_type, void* parent);

    void jalv_ui_port_event(LV2Model* jalv, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void* buffer);
//  bool jalv_send_to_ui(LV2Model* model, uint32_t port_index, uint32_t type, uint32_t size, const void* body);
//  void jalv_init_ui(LV2Model* model);
    uint32_t jalv_ui_port_index(void* const controller, const char* symbol);

//  void jalv_apply_ui_events(LV2Model* jalv, uint32_t nframes);

//  void jalv_ui_write(void* const jalv_handle, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, const void* buffer);

    bool jalv_ui_is_resizable(LV2Model* model);

    ControlID* jalv_control_by_symbol(LV2Model* model, const char* sym);

    float _sample_rate{0};

    /** Wrappers for preparing data to pass to processReplacing */
    float* _process_inputs[LV2_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[LV2_WRAPPER_MAX_N_CHANNELS];
    ChunkSampleBuffer _dummy_input{1};
    ChunkSampleBuffer _dummy_output{1};

    bool _double_mono_input{false};
    int _number_of_programs{0};

    std::string _plugin_path;

    bool show_hidden{true};

    uint32_t _buffer_size; ///< Plugin <= >UI communication buffer size

    //  VstTimeInfo _time_info;

    // This queue holds incoming midi events.
    // They are parsed and converted to lv2_evbuf content for LV2 in
    // process_audio.
    RtEventFifo _incoming_event_queue;

/* TODO: Check, can this initialization ever fail? then, make it pointer, and move construction to init();
*  Currently, this is instantiated in wrapper.
*  But if there's more than one plugin, there should not be two instances of _loader, right?
*/
    PluginLoader _loader;

    // The below are all fields used in process_audio:
    int _p = 0, _i = 0, _o = 0;
    const KeyboardRtEvent* _keyboard_event_ptr{nullptr};
    const KeyboardCommonRtEvent* _keyboard_common_event_ptr{nullptr};
    const WrappedMidiRtEvent* _wrapped_midi_event_ptr{nullptr};
    RtEvent _rt_event;
    MidiDataByte _midi_data;
    LV2_Evbuf_Iterator _lv2_evbuf_iterator;
    LV2_Atom_Object _get_ATOM;
    Port* _current_port{nullptr};
    LV2Model* _model{nullptr};

    // In process_midi_output_for_current_port, called by process_audio:
    uint32_t _midi_frames, _midi_subframes, _midi_type, _midi_size;
    uint8_t* _midi_body;
    MidiDataByte _outgoing_midi_data;
    midi::MessageType _outgoing_midi_type;
    midi::ControlChangeMessage _decoded_midi_cc_msg;
    midi::NoteOnMessage _decoded_node_on_msg;
    midi::NoteOffMessage _decoded_node_off_msg;
    midi::PitchBendMessage _decoded_pitch_bend_msg;
    midi::PolyKeyPressureMessage _decoded_poly_pressure_msg;
    midi::ChannelPressureMessage _decoded_channel_pressure_msg;
};

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
    Lv2Wrapper(HostControl host_control, const std::string& /*lv2_plugin_uri*/) :
        Processor(host_control) {}

    ProcessorReturnCode init(float sample_rate) override
    {
        return ProcessorReturnCode::ERROR;
    }

    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& /*in*/, ChunkSampleBuffer& /*out*/) override {}
};

}// end namespace lv2
}// end namespace sushi
#endif

#endif //SUSHI_LV2_PLUGIN_H