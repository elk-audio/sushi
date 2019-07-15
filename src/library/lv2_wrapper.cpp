#ifdef SUSHI_BUILD_WITH_LV2

#include <math.h>


#include <fstream>
#include <iostream>
#include <sstream>
#include <csignal>

#include "lv2_wrapper.h"

#include "logging.h"

namespace // anonymous namespace
{

static constexpr int LV2_STRING_BUFFER_SIZE = 256;
static char canDoBypass[] = "bypass";

/** These features have no data */
    static const LV2_Feature static_features[] = {
            { LV2_STATE__loadDefaultState, NULL },
            { LV2_BUF_SIZE__powerOf2BlockLength, NULL },
            { LV2_BUF_SIZE__fixedBlockLength, NULL },
            { LV2_BUF_SIZE__boundedBlockLength, NULL } };

} // anonymous namespace

namespace sushi {
namespace lv2 {

/** Return true iff Sushi supports the given feature. */
static bool
feature_is_supported(Jalv* jalv, const char* uri)
{
    if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive")) {
        return true;
    }

    for (const LV2_Feature*const* f = jalv->feature_list; *f; ++f) {
        if (!strcmp(uri, (*f)->URI)) {
            return true;
        }
    }
    return false;
}

//constexpr uint32_t SUSHI_HOST_TIME_CAPABILITIES = kVstNanosValid | kVstPpqPosValid | kVstTempoValid |
                                                 // kVstBarsValid | kVstTimeSigValid;

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

ProcessorReturnCode Lv2Wrapper::init(float sample_rate)
{
    // TODO: sanity checks on sample_rate,
    //       but these can probably better be handled on Processor::init()
    _sample_rate = sample_rate;

    auto library_handle = _loader.get_plugin_handle_from_URI(_plugin_path.c_str());

    if (library_handle == nullptr)
    {
        _cleanup();
        return ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR;
    }

    Jalv& model =_loader.getJalvModel();

    model.plugin = library_handle;

    /* Build feature list for passing to plugins */
    const LV2_Feature* const features[] = {
            &model.features.map_feature/*,
            &model.features.unmap_feature*/
// TODO Ilias: Re-introduce these or remove!
            /*,
            &model.features.sched_feature,
            &model.features.log_feature,
            &model.features.options_feature,
            &static_features[0],
            &static_features[1],
            &static_features[2],
            &static_features[3],
            NULL*/
    };

    // TODO Ilias: Dynamic cast? I should just RAII it.
    model.feature_list = static_cast<const LV2_Feature**>(calloc(1, sizeof(features)));

    if (!model.feature_list)
    {
        fprintf(stderr, "Failed to allocate feature list\n");
        //jalv_close(jalv);
        //return -7;
        // TODO Ilias: Deal with failure!
    }
    memcpy(model.feature_list, features, sizeof(features));

    /* Check that any required features are supported */
    LilvNodes* req_feats = lilv_plugin_get_required_features(model.plugin);
    LILV_FOREACH(nodes, f, req_feats) {
        const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
        if (!feature_is_supported(&model, uri)) {
            fprintf(stderr, "Feature %s is not supported\n", uri);
            /*jalv_close(jalv);
            return -8;*/
            // TODO Ilias: Deal with failure!
        }
    }
    lilv_nodes_free(req_feats);


    _loader.load_plugin(library_handle, _sample_rate, model.feature_list);

    if (model.instance == nullptr)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_ENTRY_POINT_NOT_FOUND;
    }

// Ilias TODO: Re-introduce if equivalent is found for LV2.
// Set Processor's name and label (using VST's ProductString)
//    char effect_name[VST_STRING_BUFFER_SIZE] = {0};
//    char product_string[VST_STRING_BUFFER_SIZE] = {0};

//    _vst_dispatcher(effGetEffectName, 0, 0, effect_name, 0);
//    _vst_dispatcher(effGetProductString, 0, 0, product_string, 0);
//    set_name(std::string(&effect_name[0]));
//    set_label(std::string(&product_string[0]));

// Get plugin can do:s
//    int bypass = _vst_dispatcher(effCanDo, 0, 0, canDoBypass, 0);
//    _can_do_soft_bypass = (bypass == 1);
//    _number_of_programs = _plugin_handle->numPrograms;

    // These are currently modified in the create_ports call below:
    _max_input_channels = 0;
    _max_output_channels = 0;

    create_ports(library_handle);

    // Channel setup:
    _current_input_channels = _max_input_channels;
    _current_output_channels = _max_output_channels;

    // Register internal parameters
    if (!_register_parameters())
    {
        _cleanup();
        return ProcessorReturnCode::PARAMETER_ERROR;
    }

    return ProcessorReturnCode::OK;
}

void Lv2Wrapper::create_ports(const LilvPlugin *plugin)
{
    Jalv& model = _loader.getJalvModel();

    model.num_ports = lilv_plugin_get_num_ports(plugin);
    model.ports = (struct Port*)calloc(model.num_ports, sizeof(struct Port));

    float* default_values = (float*)calloc(lilv_plugin_get_num_ports(plugin), sizeof(float));

    lilv_plugin_get_port_ranges_float(plugin, NULL, NULL, default_values);

    for (uint32_t i = 0; i < model.num_ports; ++i)
    {
        create_port(plugin, i, default_values[i]);
    }

    // The following is not done in LV2Apply.
    // And for AMP at least, it is always null.
    const LilvPort* control_input = lilv_plugin_get_port_by_designation(
            plugin, model.nodes.lv2_InputPort, model.nodes.lv2_control);
    if (control_input)
    {
        model.control_in = lilv_port_get_index(plugin, control_input);
    }

    free(default_values);
}

/**
   Create a port structure from data description. This is called before plugin
   and Jack instantiation. The remaining instance-specific setup
   (e.g. buffers) is done later in activate_port().
*/
void Lv2Wrapper::create_port(const LilvPlugin *plugin, uint32_t port_index, float default_value)
{
    Jalv& model = _loader.getJalvModel();
    struct Port* const port = &(model.ports[port_index]);

    port->lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);
    port->index = port_index;
    port->control = 0.0f;
    port->flow = FLOW_UNKNOWN;

    // The below are not used in lv2apply example.
    //port->sys_port = NULL; // For audio/MIDI ports, otherwise NULL
    port->evbuf = NULL; // For MIDI ports, otherwise NULL
    port->buf_size = 0; // Custom buffer size, or 0

    const bool optional = lilv_port_has_property(plugin, port->lilv_port, model.nodes.lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, port->lilv_port, model.nodes.lv2_InputPort))
    {
        port->flow = FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, port->lilv_port, model.nodes.lv2_OutputPort))
    {
        port->flow = FLOW_OUTPUT;
    }
    else if (!optional)
    {
// Ilias TODO: Handle error the sushi way.
        assert(false);
//      die("Mandatory port has unknown type (neither input nor output)");
    }

    const bool hidden = !show_hidden &&
                        lilv_port_has_property(plugin, port->lilv_port, model.nodes.pprops_notOnGUI);

    /* Set control values */
    if (lilv_port_is_a(plugin, port->lilv_port, model.nodes.lv2_ControlPort))
    {
        port->type = TYPE_CONTROL;

        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, port->lilv_port, &defNode, &minNode, &maxNode);

        if(defNode!=nullptr)
            port->def = lilv_node_as_float(defNode);

        if(maxNode!=nullptr)
            port->max = lilv_node_as_float(maxNode);

        if(minNode!=nullptr)
            port->min = lilv_node_as_float(minNode);

        port->control = isnan(default_value) ? port->def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);

        if (!hidden)
        {
// Ilias TODO: Re-Introduce
//          add_control(&_jalv.controls, new_port_control(_jalv, port->index));
        }
    }
    else if (lilv_port_is_a(plugin, port->lilv_port, model.nodes.lv2_AudioPort))
    {
        port->type = TYPE_AUDIO;
// Ilias TODO: Understand, maybe re-introduce.
#ifdef HAVE_JACK_METADATA
        } else if (lilv_port_is_a(jalv->plugin, port->lilv_port,
                      jalv->nodes.lv2_CVPort)) {
port->type = TYPE_CV;
#endif

    }
    else if (lilv_port_is_a(plugin, port->lilv_port, model.nodes.atom_AtomPort))
    {
        port->type = TYPE_EVENT;
    }
    else if (!optional)
    {
// Ilias TODO: Handle error the sushi way.
        assert(false);
//      die("Mandatory port has unknown data type");
    }

    if(port->type == TYPE_AUDIO) {
        if(port->flow == FLOW_INPUT)
            _max_input_channels++;
        else if(port->flow == FLOW_OUTPUT)
            _max_output_channels++;
    }

// The below min_size code is not even present in lv2apply example.

    // Todo: This is always returned NULL for amp output, should it?
    LilvNode* min_size = lilv_port_get(plugin, port->lilv_port, model.nodes.rsz_minimumSize);

    if (min_size && lilv_node_is_int(min_size))
    {
        port->buf_size = lilv_node_as_int(min_size);
        _buffer_size = MAX(_buffer_size, port->buf_size * N_BUFFER_CYCLES);
    }

    lilv_node_free(min_size);
}

//
void Lv2Wrapper::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    bool reset_enabled = enabled();
    if (reset_enabled)
    {
        set_enabled(false);
    }
    /*_vst_dispatcher(effSetSampleRate, 0, 0, 0, _sample_rate);*/
    if (reset_enabled)
    {
        set_enabled(true);
    }
    return;
}

void Lv2Wrapper::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    _update_mono_mode(valid_arr);
}

void Lv2Wrapper::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    _update_mono_mode(valid_arr);
}

void Lv2Wrapper::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    if (enabled)
    {
        /*_vst_dispatcher(effMainsChanged, 0, 1, NULL, 0.0f);
        _vst_dispatcher(effStartProcess, 0, 0, NULL, 0.0f);*/
    }
    else
    {
        /*_vst_dispatcher(effMainsChanged, 0, 0, NULL, 0.0f);
        _vst_dispatcher(effStopProcess, 0, 0, NULL, 0.0f);*/
    }
}

void Lv2Wrapper::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
    if (_can_do_soft_bypass)
    {
        /*_vst_dispatcher(effSetBypass, 0, bypassed ? 1 : 0, NULL, 0.0f);*/
    }
}

std::pair<ProcessorReturnCode, float> Lv2Wrapper::parameter_value(ObjectId parameter_id) const
{
    /*if (static_cast<int>(parameter_id) < _plugin_handle->numParams)
    {
        float value = _plugin_handle->getParameter(_plugin_handle, static_cast<VstInt32>(parameter_id));
        return {ProcessorReturnCode::OK, value};
    }*/
    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
}

std::pair<ProcessorReturnCode, float> Lv2Wrapper::parameter_value_normalised(ObjectId parameter_id) const
{
    return this->parameter_value(parameter_id);
}

std::pair<ProcessorReturnCode, std::string> Lv2Wrapper::parameter_value_formatted(ObjectId parameter_id) const
{
    /*if (static_cast<int>(parameter_id) < _plugin_handle->numParams)
    {
        char buffer[kVstMaxParamStrLen] = "";
        _vst_dispatcher(effGetParamDisplay, 0, static_cast<VstInt32>(parameter_id), buffer, 0);
        return {ProcessorReturnCode::OK, buffer};
    }*/
    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
}

int Lv2Wrapper::current_program() const
{
    /*if (this->supports_programs())
    {
        return _vst_dispatcher(effGetProgram, 0, 0, nullptr, 0);
    }*/
    return 0;
}

std::string Lv2Wrapper::current_program_name() const
{
    /*if (this->supports_programs())
    {
        char buffer[kVstMaxProgNameLen] = "";
        _vst_dispatcher(effGetProgramName, 0, 0, buffer, 0);
        return std::string(buffer);
    }
*/    return "";
}

std::pair<ProcessorReturnCode, std::string> Lv2Wrapper::program_name(int program) const
{
    /*if (this->supports_programs())
    {
        char buffer[kVstMaxProgNameLen] = "";
        auto success = _vst_dispatcher(effGetProgramNameIndexed, program, 0, buffer, 0);
        return {success ? ProcessorReturnCode::OK : ProcessorReturnCode::PARAMETER_NOT_FOUND, buffer};
    }*/
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, ""};
}

std::pair<ProcessorReturnCode, std::vector<std::string>> Lv2Wrapper::all_program_names() const
{
    if (!this->supports_programs())
    {
        return {ProcessorReturnCode::UNSUPPORTED_OPERATION, std::vector<std::string>()};
    }
    std::vector<std::string> programs;
    /*for (int i = 0; i < _number_of_programs; ++i)
    {
        char buffer[kVstMaxProgNameLen] = "";
        _vst_dispatcher(effGetProgramNameIndexed, i, 0, buffer, 0);
        programs.push_back(buffer);
    }*/
    return {ProcessorReturnCode::OK, programs};
}

ProcessorReturnCode Lv2Wrapper::set_program(int program)
{
    if (this->supports_programs() && program < _number_of_programs)
    {
       /* _vst_dispatcher(effBeginSetProgram, 0, 0, nullptr, 0);
        *//* Vst2 lacks a mechanism for signaling that the program change was successful *//*
        _vst_dispatcher(effSetProgram, 0, program, nullptr, 0);
        _vst_dispatcher(effEndSetProgram, 0, 0, nullptr, 0);
        return ProcessorReturnCode::OK;*/
    }
    return ProcessorReturnCode::UNSUPPORTED_OPERATION;
}

void Lv2Wrapper::_cleanup()
{
    // Tell plugin to stop and shutdown
    set_enabled(false);

    _loader.close_plugin_instance();
}

bool Lv2Wrapper::_register_parameters()
{
    bool param_inserted_ok = true;

    Jalv& model = _loader.getJalvModel();

    for (int _pi = 0; _pi < model.num_ports; ++_pi)
    {
        const Port& currentPort = model.ports[_pi];

        if (currentPort.type == TYPE_CONTROL)
        {
            // Here I need to get the name of the port.
            auto nameNode = lilv_port_get_name(model.plugin, currentPort.lilv_port);

            std::string nameAsString = lilv_node_as_string(nameNode);

            param_inserted_ok = register_parameter(new FloatParameterDescriptor(nameAsString, // name
                    nameAsString, // label
                    currentPort.min, // range min
                    currentPort.max, // range max
                    nullptr), // ParameterPreProcessor
                    static_cast<ObjectId>(_pi)); // Registering the ObjectID as the index in LV2 plugin's ports list.

            if (param_inserted_ok)
            {
                MIND_LOG_DEBUG("Plugin: {}, registered param: {}", name(), nameAsString);
            }
            else
            {
                MIND_LOG_ERROR("Plugin: {}, Error while registering param: {}", name(), nameAsString);
            }

            lilv_node_free(nameNode);
        }
    }

    return param_inserted_ok;
}

void Lv2Wrapper::process_event(RtEvent event)
{
    if (event.type() == RtEventType::FLOAT_PARAMETER_CHANGE)
    {
        auto typed_event = event.parameter_change_event();
        auto id = typed_event->param_id();

        std::cout << "Parameter, ID: " << id << ", value: " << typed_event->value() << std::endl;

        int portIndex = static_cast<int>(id);
        assert( portIndex < _loader.getJalvModel().num_ports);

        _loader.getJalvModel().ports[portIndex].control = typed_event->value();
    }
    else if (is_keyboard_event(event))
    {
/*
        if (_vst_midi_events_fifo.push(event) == false)
        {
            MIND_LOG_WARNING("Plugin: {}, MIDI queue Overflow!", name());
        }
*/
    }
    else
    {
        MIND_LOG_INFO("Plugin: {}, received unhandled event", name());
    }
}

void Lv2Wrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypassed && !_can_do_soft_bypass)
    {
        bypass_process(in_buffer, out_buffer);
// TODO Ilias: Re-instate
//      _vst_midi_events_fifo.flush();
    }
    else
    {
// TODO Ilias: Re-instate
        /*_vst_dispatcher(effProcessEvents, 0, 0, _vst_midi_events_fifo.flush(), 0.0f);*/

        _map_audio_buffers(in_buffer, out_buffer);

        Jalv& model = _loader.getJalvModel();

        for (_p = 0, _i = 0, _o = 0; _p < _loader.getJalvModel().num_ports; ++_p)
        {
            if (model.ports[_p].type == TYPE_CONTROL)
            {
                lilv_instance_connect_port(model.instance, _p, &model.ports[_p].control);
            }
            else if (model.ports[_p].type == TYPE_AUDIO)
            {
                if (model.ports[_p].flow == FLOW_INPUT)
                {
                    lilv_instance_connect_port(model.instance, _p, _process_inputs[_i++]);
                }
                else
                {
                    lilv_instance_connect_port(model.instance, _p, _process_outputs[_o++]);
                }
            }
            else
            {
                lilv_instance_connect_port(model.instance, _p, NULL);
            }
        }

        lilv_instance_run(model.instance, AUDIO_CHUNK_SIZE);
    }
}

/*void Lv2Wrapper::notify_parameter_change_rt(VstInt32 parameter_index, float value)
{
    *//* The default Vst2.4 implementation calls set_parameter() in set_parameter_automated()
     * so the plugin is already aware of the change, we just need to send a notification
     * to the non-rt part *//*
    *//*if (parameter_index > this->parameter_count())
    {
        return;
    }
    auto e = RtEvent::make_parameter_change_event(this->id(), 0, static_cast<ObjectId>(parameter_index), value);
    output_event(e);*//*
}

void Lv2Wrapper::notify_parameter_change(VstInt32 parameter_index, float value)
{
    *//*auto e = new ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT,
                                                  this->id(),
                                                  static_cast<ObjectId>(parameter_index),
                                                  value,
                                                  IMMEDIATE_PROCESS);
    _host_control.post_event(e);*//*
}*/

bool Lv2Wrapper::_update_speaker_arrangements(int inputs, int outputs)
{
    /*VstSpeakerArrangement in_arr;
    VstSpeakerArrangement out_arr;
    in_arr.numChannels = inputs;
    in_arr.type = arrangement_from_channels(inputs);
    out_arr.numChannels = outputs;
    out_arr.type = arrangement_from_channels(outputs);
    int res = _vst_dispatcher(effSetSpeakerArrangement, 0, (VstIntPtr)&in_arr, &out_arr, 0);
    return res == 1;*/
    return false;
}

/*VstTimeInfo* Lv2Wrapper::time_info()
{
    auto transport = _host_control.transport();
    auto ts = transport->current_time_signature();

    *//*_time_info.samplePos          = transport->current_samples();
    _time_info.sampleRate         = _sample_rate;
    _time_info.nanoSeconds        = std::chrono::duration_cast<std::chrono::nanoseconds>(transport->current_process_time()).count();
    _time_info.ppqPos             = transport->current_beats();
    _time_info.tempo              = transport->current_tempo();
    _time_info.barStartPos        = transport->current_bar_start_beats();
    _time_info.timeSigNumerator   = ts.numerator;
    _time_info.timeSigDenominator = ts.denominator;
    _time_info.flags = SUSHI_HOST_TIME_CAPABILITIES | transport->playing()? kVstTransportPlaying : 0;*//*
    return &_time_info;
}*/

void Lv2Wrapper::_map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    int i;
    if (_double_mono_input)
    {
        _process_inputs[0] = const_cast<float*>(in_buffer.channel(0));
        _process_inputs[1] = const_cast<float*>(in_buffer.channel(0));
    }
    else
    {
        for (i = 0; i < _current_input_channels; ++i)
        {
            _process_inputs[i] = const_cast<float*>(in_buffer.channel(i));
        }
        for (; i <= _max_input_channels; ++i)
        {
            _process_inputs[i] = (_dummy_input.channel(0));
        }
    }
    for (i = 0; i < _current_output_channels; i++)
    {
        _process_outputs[i] = out_buffer.channel(i);
    }
    for (; i <= _max_output_channels; ++i)
    {
        _process_outputs[i] = _dummy_output.channel(0);
    }
}

void Lv2Wrapper::_update_mono_mode(bool speaker_arr_status)
{
    _double_mono_input = false;
    if (speaker_arr_status)
    {
        return;
    }
    if (_current_input_channels == 1 && _max_input_channels == 2)
    {
        _double_mono_input = true;
    }
}

/*VstSpeakerArrangementType arrangement_from_channels(int channels)
{
    switch (channels)
    {
        case 0:
            return kSpeakerArrEmpty;
        case 1:
            return kSpeakerArrMono;
        case 2:
            return kSpeakerArrStereo;
        case 3:
            return kSpeakerArr30Music;
        case 4:
            return kSpeakerArr40Music;
        case 5:
            return kSpeakerArr50;
        case 6:
            return kSpeakerArr60Music;
        case 7:
            return kSpeakerArr70Music;
        default:
            return kSpeakerArr80Music; //TODO - decide how to handle multichannel setups
    }
    return kNumSpeakerArr;
}*/

} // namespace lv2
} // namespace sushi

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2
#include "lv2_wrapper.h"
#include "logging.h"
namespace sushi {
namespace lv2 {
MIND_GET_LOGGER;
ProcessorReturnCode Lv2Wrapper::init(float /*sample_rate*/)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with LV2 support!");
    return ProcessorReturnCode::ERROR;
}}}
#endif