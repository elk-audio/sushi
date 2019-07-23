#ifdef SUSHI_BUILD_WITH_LV2

#include <math.h>


#include <fstream>
#include <iostream>
#include <sstream>
#include <csignal>

#include "lv2_wrapper.h"

#include "logging.h"

namespace
{

static constexpr int LV2_STRING_BUFFER_SIZE = 256;

// TODO: Ilias - introduce these.
/** These features have no data */
    static const LV2_Feature static_features[] = {
            { LV2_STATE__loadDefaultState, NULL },
            { LV2_BUF_SIZE__powerOf2BlockLength, NULL },
            { LV2_BUF_SIZE__fixedBlockLength, NULL },
            { LV2_BUF_SIZE__boundedBlockLength, NULL } };

} // anonymous namespace

namespace sushi {
namespace lv2 {

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

/** Return true iff Sushi supports the given feature. */
static bool feature_is_supported(LV2Model* model, const char* uri)
{
    if (!strcmp(uri, "http://lv2plug.in/ns/lv2core#isLive")) {
        return true;
    }

    for (const LV2_Feature*const* f = model->feature_list; *f; ++f) {
        if (!strcmp(uri, (*f)->URI)) {
            return true;
        }
    }
    return false;
}

void Lv2Wrapper::_allocate_port_buffers(LV2Model *model)
{
    for (int i = 0; i < model->num_ports; ++i)
    {
        struct Port* const port = &model->ports[i];

        switch (port->type)
        {
            case TYPE_EVENT: {
                lv2_evbuf_free(port->evbuf);
                const size_t buf_size = (port->buf_size > 0)
                                        ? port->buf_size
                                        : model->midi_buf_size;
                port->evbuf = lv2_evbuf_new(
                        buf_size,
                        model->map.map(model->map.handle,
                                      lilv_node_as_string(model->nodes.atom_Chunk)),
                        model->map.map(model->map.handle,
                                      lilv_node_as_string(model->nodes.atom_Sequence)));
                lilv_instance_connect_port(
                        model->instance, i, lv2_evbuf_get_buffer(port->evbuf));
            }
            default:
                break;
        }
    }
}

ProcessorReturnCode Lv2Wrapper::init(float sample_rate)
{
    // TODO: sanity checks on sample_rate,
    //       but these can probably better be handled on Processor::init()
    _sample_rate = sample_rate;

    auto library_handle = _loader.get_plugin_handle_from_URI(_plugin_path.c_str());

    if (library_handle == nullptr)
    {
        MIND_LOG_ERROR("Failed to load LV2 plugin - handle not recognized.");
        _cleanup();
        return ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR;
    }

    _model = _loader.getModel();

    _model->plugin = library_handle;

    /* Build feature list for passing to plugins */
    const LV2_Feature* const features[] = {
            &_model->features.map_feature,
            &_model->features.unmap_feature,
            &_model->features.log_feature,
// TODO Ilias: Re-introduce these or remove!
            /*
            &model.features.sched_feature,
            &model.features.options_feature,
            */
            &static_features[0],
            &static_features[1],
            &static_features[2],
            &static_features[3],
            nullptr
    };

    // TODO Ilias: I should just RAII it.
    _model->feature_list = static_cast<const LV2_Feature**>(calloc(1, sizeof(features)));

    if (!_model->feature_list)
    {
        MIND_LOG_ERROR("Failed to allocate LV2 feature list.");
        _cleanup();
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    memcpy(_model->feature_list, features, sizeof(features));

    /* Check that any required features are supported */
    LilvNodes* req_feats = lilv_plugin_get_required_features(_model->plugin);
    LILV_FOREACH(nodes, f, req_feats)
    {
        const char* uri = lilv_node_as_uri(lilv_nodes_get(req_feats, f));
        if (!feature_is_supported(_model, uri))
        {
            MIND_LOG_ERROR("LV2 feature {} is not supported\n", uri);
            _cleanup();
            return ProcessorReturnCode::PLUGIN_INIT_ERROR;
        }
    }
    lilv_nodes_free(req_feats);

    _loader.load_plugin(library_handle, _sample_rate, _model->feature_list);

    if (_model->instance == nullptr)
    {
        MIND_LOG_ERROR("Failed to load LV2 - Plugin entry point not found.");
        _cleanup();
        return ProcessorReturnCode::PLUGIN_ENTRY_POINT_NOT_FOUND;
    }

    const LilvNode* uri_node = lilv_plugin_get_uri(_model->plugin);
    const std::string uri_as_string = lilv_node_as_string(uri_node);
    set_name(uri_as_string);

    LilvNode* label_node = lilv_plugin_get_name(_model->plugin);
    const std::string label_as_string = lilv_node_as_string(label_node);
    set_label(label_as_string);
    lilv_free(label_node);

// Ilias TODO: Re-introduce if equivalent is found for LV2.
//  _number_of_programs = _plugin_handle->numPrograms;

    // These are currently modified in the _create_ports call below:
    _max_input_channels = 0;
    _max_output_channels = 0;

    _create_ports(library_handle);

    // Channel setup:
    _current_input_channels = _max_input_channels;
    _current_output_channels = _max_output_channels;

    // Register internal parameters
    if (!_register_parameters())
    {
        MIND_LOG_ERROR("Failed to allocate LV2 feature list.");
        _cleanup();
        return ProcessorReturnCode::PARAMETER_ERROR;
    }

    return ProcessorReturnCode::OK;
}

void Lv2Wrapper::_create_ports(const LilvPlugin *plugin)
{
    _model->num_ports = lilv_plugin_get_num_ports(plugin);
    _model->ports = (struct Port*)calloc(_model->num_ports, sizeof(struct Port));

    float* default_values = (float*)calloc(lilv_plugin_get_num_ports(plugin), sizeof(float));

    lilv_plugin_get_port_ranges_float(plugin, NULL, NULL, default_values);

    for (int i = 0; i < _model->num_ports; ++i)
    {
        _create_port(plugin, i, default_values[i]);
    }

    // TODO: Ilias - find a plugin where the control_input isn't null.
    const LilvPort* control_input = lilv_plugin_get_port_by_designation(
            plugin, _model->nodes.lv2_InputPort, _model->nodes.lv2_control);
    if (control_input)
    {
        _model->control_in = lilv_port_get_index(plugin, control_input);
    }

    free(default_values);


    if (!_model->buf_size_set) {
        _allocate_port_buffers(_model);
    }
}

/**
   Create a port structure from data description. This is called before plugin
   and Jack instantiation. The remaining instance-specific setup
   (e.g. buffers) is done later in activate_port().
*/
void Lv2Wrapper::_create_port(const LilvPlugin *plugin, int port_index, float default_value)
{
    struct Port* const port = &(_model->ports[port_index]);

    port->lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);
    port->index = port_index;
    port->control = 0.0f;
    port->flow = FLOW_UNKNOWN;

    // The below are not used in lv2apply example.
    //port->sys_port = NULL; // For audio/MIDI ports, otherwise NULL

    port->evbuf = nullptr; // For MIDI ports, otherwise NULL

    port->buf_size = 0; // Custom buffer size, or 0

    const bool optional = lilv_port_has_property(plugin, port->lilv_port, _model->nodes.lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, port->lilv_port, _model->nodes.lv2_InputPort))
    {
        port->flow = FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, port->lilv_port, _model->nodes.lv2_OutputPort))
    {
        port->flow = FLOW_OUTPUT;
    }
    else if (!optional)
    {
        assert(false);
        MIND_LOG_ERROR("Mandatory LV2 port has unknown type (neither input nor output)");
        _cleanup();
    }

    // TODO: Re-introduce once 'GUI' is implemented.
/*    const bool hidden = !show_hidden &&
                        lilv_port_has_property(plugin, port->lilv_port, _model->nodes.pprops_notOnGUI);*/

    /* Set control values */
    if (lilv_port_is_a(plugin, port->lilv_port, _model->nodes.lv2_ControlPort))
    {
        port->type = TYPE_CONTROL;

        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, port->lilv_port, &defNode, &minNode, &maxNode);

        if(defNode != nullptr)
            port->def = lilv_node_as_float(defNode);

        if(maxNode != nullptr)
            port->max = lilv_node_as_float(maxNode);

        if(minNode != nullptr)
            port->min = lilv_node_as_float(minNode);

        port->control = isnan(default_value) ? port->def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);
    }
    else if (lilv_port_is_a(plugin, port->lilv_port, _model->nodes.lv2_AudioPort))
    {
        port->type = TYPE_AUDIO;

// Ilias TODO: CV port(s).
//#ifdef HAVE_JACK_METADATA
//        } else if (lilv_port_is_a(model->plugin, port->lilv_port,
//                      model->nodes.lv2_CVPort)) {
//port->type = TYPE_CV;
//#endif

    }
    else if (lilv_port_is_a(plugin, port->lilv_port, _model->nodes.atom_AtomPort))
    {
        port->type = TYPE_EVENT;
    }
    else if (!optional)
    {
        assert(false);
        _cleanup();
        assert(false);
        MIND_LOG_ERROR("Mandatory LV2 port has unknown data type");
        _cleanup();
    }

    if(port->type == TYPE_AUDIO) {
        if(port->flow == FLOW_INPUT)
            _max_input_channels++;
        else if(port->flow == FLOW_OUTPUT)
            _max_output_channels++;
    }

    // Todo: This is always returned NULL for amp output, should it?
    LilvNode* min_size = lilv_port_get(plugin, port->lilv_port, _model->nodes.rsz_minimumSize);

    if (min_size && lilv_node_is_int(min_size))
    {
        port->buf_size = lilv_node_as_int(min_size);
        _buffer_size = MAX(_buffer_size, port->buf_size * N_BUFFER_CYCLES);
    }

    lilv_node_free(min_size);
}


void Lv2Wrapper::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    bool reset_enabled = enabled();

    if (reset_enabled)
    {
        set_enabled(false);
    }

    if (reset_enabled)
    {
        set_enabled(true);
    }

    return;
}

std::pair<ProcessorReturnCode, float> Lv2Wrapper::parameter_value(ObjectId parameter_id) const
{
    float value = 0.0;
    const int index = static_cast<int>(parameter_id);
    if (index < _model->num_ports)
    {
        struct Port* port = &_model->ports[index];
        if (port)
        {
            value = port->control;
            return {ProcessorReturnCode::OK, value};
        }
    }
    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, value};
}

std::pair<ProcessorReturnCode, float> Lv2Wrapper::parameter_value_normalised(ObjectId parameter_id) const
{
    // TODO:  Ilias - Implement normalization
    return this->parameter_value(parameter_id);
}

std::pair<ProcessorReturnCode, std::string> Lv2Wrapper::parameter_value_formatted(ObjectId parameter_id) const
{
    // TODO:  Ilias - Populate
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

    for (int _pi = 0; _pi < _model->num_ports; ++_pi)
    {
        const Port& currentPort = _model->ports[_pi];

        if (currentPort.type == TYPE_CONTROL)
        {
            // Here I need to get the name of the port.
            auto nameNode = lilv_port_get_name(_model->plugin, currentPort.lilv_port);

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

        const int portIndex = static_cast<int>(id);
        assert(portIndex < _model->num_ports);

        struct Port* port = &(_model->ports[portIndex]);

        port->control = typed_event->value();
    }
    else if (is_keyboard_event(event))
    {
        if (_incoming_event_queue.push(event) == false)
        {
            MIND_LOG_WARNING("Plugin: {}, MIDI queue Overflow!", name());
        }
    }
    else
    {
        MIND_LOG_INFO("Plugin: {}, received unhandled event", name());
    }
}

void Lv2Wrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypassed)
    {
         bypass_process(in_buffer, out_buffer);
        _flush_event_queue();
    }
    else
    {
        _map_audio_buffers(in_buffer, out_buffer);

        _deliver_inputs_to_plugin();

        lilv_instance_run(_model->instance, AUDIO_CHUNK_SIZE);

        _deliver_outputs_from_plugin();
    }
}

void Lv2Wrapper::_deliver_inputs_to_plugin()
{
    for (_p = 0, _i = 0, _o = 0; _p < _model->num_ports; ++_p)
    {
        _current_port = &(_model->ports[_p]);

        switch(_current_port->type)
        {
            case TYPE_CONTROL:
                lilv_instance_connect_port(_model->instance, _p, &_current_port->control);
                break;
            case TYPE_AUDIO:
                if (_current_port->flow == FLOW_INPUT)
                    lilv_instance_connect_port(_model->instance, _p, _process_inputs[_i++]);
                else
                    lilv_instance_connect_port(_model->instance, _p, _process_outputs[_o++]);
                break;
            case TYPE_EVENT:
                if (_current_port->flow == FLOW_INPUT)
                {
                    lv2_evbuf_reset(_current_port->evbuf, true);
                    _process_midi_input_for_current_port();

                }
                else if (_current_port->flow == FLOW_OUTPUT)// Clear event output for plugin to write to.
                {
                    lv2_evbuf_reset(_current_port->evbuf, false);
                }
                break;
            case TYPE_CV: // TODO Ilias: Implement also CV support.
            case TYPE_UNKNOWN:
                assert(false);
                break;
            default:
                lilv_instance_connect_port(_model->instance, _p, NULL);
        }
    }
    _model->request_update = false;
}

void Lv2Wrapper::_deliver_outputs_from_plugin()
{
    for (_p = 0; _p < _model->num_ports; ++_p)
    {
        _current_port = &(_model->ports[_p]);
        if(_current_port->flow == FLOW_OUTPUT)
        {
            switch(_current_port->type)
            {
                case TYPE_CONTROL:
                    if (lilv_port_has_property(_model->plugin,
                                               _current_port->lilv_port,
                                               _model->nodes.lv2_reportsLatency))
                    {
                        if (_model->plugin_latency != _current_port->control)
                        {
                            _model->plugin_latency = _current_port->control;
                            // TODO: introduce latency compensation reporting to Sushi
                        }
                    }
                    break;
                case TYPE_EVENT:
                    _process_midi_output_for_current_port();
                    break;
            }
        }
    }
}

void Lv2Wrapper::_process_midi_output_for_current_port()
{
    for (LV2_Evbuf_Iterator buf_i = lv2_evbuf_begin(_current_port->evbuf);
         lv2_evbuf_is_valid(buf_i);
         buf_i = lv2_evbuf_next(buf_i))
    {
        // Get event from LV2 buffer

        lv2_evbuf_get(buf_i, &_midi_frames, &_midi_subframes, &_midi_type, &_midi_size, &_midi_body);

        // TODO: WHY SHOULD THIS BE NEEDED? to_midi_data_byte expects size to be 3 with an Assertion.
        _midi_size--;

        if (_midi_type == _model->urids.midi_MidiEvent)
        {
            _outgoing_midi_data = midi::to_midi_data_byte(_midi_body, _midi_size);
            _outgoing_midi_type = midi::decode_message_type(_outgoing_midi_data);

            switch (_outgoing_midi_type)
            {
                case midi::MessageType::CONTROL_CHANGE:
                {
                    _decoded_midi_cc_msg = midi::decode_control_change(_outgoing_midi_data);
                    output_event(RtEvent::make_parameter_change_event(0, // Target 0 here - not sure why. Same as in VST3.
                                                                      _decoded_midi_cc_msg.channel,
                                                                      _decoded_midi_cc_msg.controller,
                                                                      _decoded_midi_cc_msg.value));
                    break;
                }
                case midi::MessageType::NOTE_ON:
                {
                    _decoded_node_on_msg = midi::decode_note_on(_outgoing_midi_data);
                    output_event(RtEvent::make_note_on_event(0,
                                                             0, // Sample offset 0?
                                                             _decoded_node_on_msg.channel,
                                                             _decoded_node_on_msg.note,
                                                             _decoded_node_on_msg.velocity));
                    break;
                }
                case midi::MessageType::NOTE_OFF:
                {
                    _decoded_node_off_msg = midi::decode_note_off(_outgoing_midi_data);
                    output_event(RtEvent::make_note_off_event(0,
                                                              0, // Sample offset 0?
                                                              _decoded_node_off_msg.channel,
                                                              _decoded_node_off_msg.note,
                                                              _decoded_node_off_msg.velocity));
                    break;
                }
                case midi::MessageType::PITCH_BEND:
                {
                    _decoded_pitch_bend_msg = midi::decode_pitch_bend(_outgoing_midi_data);
                    output_event(RtEvent::make_pitch_bend_event(0,
                                                                0, // Sample offset 0?
                                                                _decoded_pitch_bend_msg.channel,
                                                                _decoded_pitch_bend_msg.value));
                    break;
                }
                case midi::MessageType::POLY_KEY_PRESSURE:
                {
                    _decoded_poly_pressure_msg = midi::decode_poly_key_pressure(_outgoing_midi_data);
                    output_event(RtEvent::make_note_aftertouch_event(0,
                                                                     0, // Sample offset 0?
                                                                     _decoded_poly_pressure_msg.channel,
                                                                     _decoded_poly_pressure_msg.note,
                                                                     _decoded_poly_pressure_msg.pressure));
                    break;
                }
                case midi::MessageType::CHANNEL_PRESSURE:
                {
                    _decoded_channel_pressure_msg = midi::decode_channel_pressure(_outgoing_midi_data);
                    output_event(RtEvent::make_aftertouch_event(0,
                                                                0, // Sample offset 0?
                                                                _decoded_channel_pressure_msg.channel,
                                                                _decoded_channel_pressure_msg.pressure));
                    break;
                }
                default:
                    output_event(RtEvent::make_wrapped_midi_event(0,
                                                                  0, // Sample offset 0?
                                                                  _outgoing_midi_data));
                    break;
            }
        }

        /*if (model->has_ui) {
            // Forward event to UI
            jalv_send_to_ui(model, p, type, size, body);
        }*/
    }
}

void Lv2Wrapper::_process_midi_input_for_current_port()
{
    _lv2_evbuf_iterator = lv2_evbuf_begin(_current_port->evbuf);

    // TODO: Re-introduce transport support.
    /* Write transport change event if applicable */
    /*
    if (xport_changed)
    {
        lv2_evbuf_write(&_lv2_evbuf_iterator, 0, 0,
                        lv2_pos->type, lv2_pos->size,
                        (const uint8_t*)LV2_ATOM_BODY(lv2_pos));
    }*/

    // TODO: This will never run for now! will be used for having 'UI' support.
    // with LV2Model example. Request_update is never set to true.
    if (_model->request_update)
    {
        /* Plugin state has changed, request an update */
        _get_ATOM = {
                {sizeof(LV2_Atom_Object_Body), _model->urids.atom_Object},
                {0,                            _model->urids.patch_Get}};

        lv2_evbuf_write(&_lv2_evbuf_iterator, 0, 0,
                        _get_ATOM.atom.type, _get_ATOM.atom.size,
                        (const uint8_t *) LV2_ATOM_BODY(&_get_ATOM));
    }

    // MIDI transfer, from incoming RT event queue into LV2 event buffers:
    while (!_incoming_event_queue.empty())
    {
        if (_incoming_event_queue.pop(_rt_event))
        {
            _convert_event_to_midi_buffer(_rt_event);

            lv2_evbuf_write(&_lv2_evbuf_iterator,
                            _rt_event.sample_offset(), // Is sample_offset the timestamp?
                            0, // Subframes
                            _model->urids.midi_MidiEvent,
                            _midi_data.size(),
                            _midi_data.data());
        }
    }
}

void Lv2Wrapper::_flush_event_queue()
{
    while (!_incoming_event_queue.empty())
    {
        _incoming_event_queue.pop(_rt_event);
    }
}

void Lv2Wrapper::_convert_event_to_midi_buffer(RtEvent& event)
{
    if (event.type() >= RtEventType::NOTE_ON && event.type() <= RtEventType::NOTE_AFTERTOUCH)
    {
        _keyboard_event_ptr = event.keyboard_event();

        switch (_keyboard_event_ptr->type())
        {
            case RtEventType::NOTE_ON:
            {
                _midi_data = midi::encode_note_on(_keyboard_event_ptr->channel(),
                                                  _keyboard_event_ptr->note(),
                                                  _keyboard_event_ptr->velocity());
                break;
            }
            case RtEventType::NOTE_OFF:
            {
                _midi_data = midi::encode_note_off(_keyboard_event_ptr->channel(),
                                                   _keyboard_event_ptr->note(),
                                                   _keyboard_event_ptr->velocity());
                break;
            }
            case RtEventType::NOTE_AFTERTOUCH:
            {
                _midi_data = midi::encode_poly_key_pressure(_keyboard_event_ptr->channel(),
                                                            _keyboard_event_ptr->note(),
                                                            _keyboard_event_ptr->velocity());
                break;
            }
        }
    }
    else if (event.type() >= RtEventType::PITCH_BEND && event.type() <= RtEventType::MODULATION)
    {
        _keyboard_common_event_ptr = event.keyboard_common_event();

        switch (_keyboard_common_event_ptr->type())
        {
            case RtEventType::AFTERTOUCH:
            {
                _midi_data = midi::encode_channel_pressure(_keyboard_common_event_ptr->channel(),
                                                           _keyboard_common_event_ptr->value());
                break;
            }
            case RtEventType::PITCH_BEND:
            {
                _midi_data = midi::encode_pitch_bend(_keyboard_common_event_ptr->channel(),
                                                     _keyboard_common_event_ptr->value());
                break;
            }
            case RtEventType::MODULATION:
            {
                _midi_data = midi::encode_control_change(_keyboard_common_event_ptr->channel(),
                                                         midi::MOD_WHEEL_CONTROLLER_NO,
                                                         _keyboard_common_event_ptr->value());
                break;
            }
        }
    }
    else if (event.type() == RtEventType::WRAPPED_MIDI_EVENT)
    {
        _wrapped_midi_event_ptr = event.wrapped_midi_event();
        _midi_data = _wrapped_midi_event_ptr->midi_data();
    }
    else
    {
        assert(false);
    }
}

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