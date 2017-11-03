#include "library/vst2x_wrapper.h"

#include "logging.h"

#include <algorithm>

namespace {

static constexpr int VST_STRING_BUFFER_SIZE = 256;
static char canDoBypass[] = "bypass";

} // anonymous namespace

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER;

ProcessorReturnCode Vst2xWrapper::init(float sample_rate)
{
    // TODO: sanity checks on sample_rate,
    //       but these can probably better be handled on Processor::init()
    _sample_rate = sample_rate;

    // Load shared library and VsT struct
    _library_handle = PluginLoader::get_library_handle_for_plugin(_plugin_path);
    if (_library_handle == nullptr)
    {
        _cleanup();
        return ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR;
    }
    _plugin_handle = PluginLoader::load_plugin(_library_handle);
    if (_plugin_handle == nullptr)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_ENTRY_POINT_NOT_FOUND;
    }

    // Check plugin's magic number
    // If incorrect, then the file either was not loaded properly, is not a
    // real VST2 plugin, or is otherwise corrupt.
    if(_plugin_handle->magic != kEffectMagic)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_LOAD_ERROR;
    }

    // Set Processor's name and label (using VST's ProductString)
    char effect_name[VST_STRING_BUFFER_SIZE] = {0};
    char product_string[VST_STRING_BUFFER_SIZE] = {0};

    _vst_dispatcher(effGetEffectName, 0, 0, effect_name, 0);
    _vst_dispatcher(effGetProductString, 0, 0, product_string, 0);
    set_name(std::string(&effect_name[0]));
    set_label(std::string(&product_string[0]));

    // Get plugin can do:s
    int bypass = _vst_dispatcher(effCanDo, 0, 0, canDoBypass, 0);
    _can_do_soft_bypass = (bypass == 1);

    // Channel setup
    _max_input_channels = _plugin_handle->numInputs;
    _current_input_channels = _max_input_channels;
    _max_output_channels = _plugin_handle->numOutputs;
    _current_output_channels = _max_output_channels;

    // Initialize internal plugin
    _vst_dispatcher(effOpen, 0, 0, 0, 0);
    _vst_dispatcher(effSetSampleRate, 0, 0, 0, _sample_rate);
    _vst_dispatcher(effSetBlockSize, 0, AUDIO_CHUNK_SIZE, 0, 0);

    // Register internal parameters
    if (!_register_parameters())
    {
        _cleanup();
        return ProcessorReturnCode::PARAMETER_ERROR;
    }

    return ProcessorReturnCode::OK;
}

void Vst2xWrapper::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    bool reset_enabled = enabled();
    if (reset_enabled)
    {
        set_enabled(false);
    }
    _vst_dispatcher(effSetSampleRate, 0, 0, 0, _sample_rate);
    if (reset_enabled)
    {
        set_enabled(true);
    }
    return;
}

void Vst2xWrapper::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    _update_mono_mode(valid_arr);
}

void Vst2xWrapper::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    _update_mono_mode(valid_arr);
}


void Vst2xWrapper::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    if (enabled)
    {
        _vst_dispatcher(effMainsChanged, 0, 1, NULL, 0.0f);
        _vst_dispatcher(effStartProcess, 0, 0, NULL, 0.0f);
    }
    else
    {
        _vst_dispatcher(effMainsChanged, 0, 0, NULL, 0.0f);
        _vst_dispatcher(effStopProcess, 0, 0, NULL, 0.0f);
    }
}

void Vst2xWrapper::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
    if (_can_do_soft_bypass)
    {
        _vst_dispatcher(effSetBypass, 0, bypassed ? 1 : 0, NULL, 0.0f);
    }
}

void Vst2xWrapper::_cleanup()
{
    if (_plugin_handle != nullptr)
    {
        // Tell plugin to stop and shutdown
        set_enabled(false);
        _vst_dispatcher(effClose, 0, 0, 0, 0);
        _plugin_handle = nullptr;
    }
    if (_library_handle != nullptr)
    {
        PluginLoader::close_library_handle(_library_handle);
    }
}

bool Vst2xWrapper::_register_parameters()
{
    char param_name[VST_STRING_BUFFER_SIZE] = {0};

    VstInt32 idx = 0;
    bool param_inserted_ok = true;
    while (param_inserted_ok && (idx < _plugin_handle->numParams) )
    {
        // TODO - query for some more properties here eventually
        _vst_dispatcher(effGetParamName, idx, 0, param_name, 0);
        param_inserted_ok = register_parameter(new FloatParameterDescriptor(param_name, param_name, 0, 1, nullptr));
        if (param_inserted_ok)
        {
            MIND_LOG_DEBUG("VsT wrapper, plugin: {}, registered param: {}", name(), param_name);
        }
        else
        {
            MIND_LOG_ERROR("VsT wrapper, plugin: {}, Error while registering param: {}", name(), param_name);
        }
        idx++;
    }

    return param_inserted_ok;
}

void Vst2xWrapper::process_event(Event event)
{
    switch (event.type())
    {
    case EventType::FLOAT_PARAMETER_CHANGE:
    {
        auto typed_event = event.parameter_change_event();
        auto id = typed_event->param_id();
        assert(static_cast<VstInt32>(id) < _plugin_handle->numParams);
        _plugin_handle->setParameter(_plugin_handle, static_cast<VstInt32>(id), typed_event->value());
    }
    break;

    case EventType::NOTE_ON:
    case EventType::NOTE_OFF:
    case EventType::NOTE_AFTERTOUCH:
    case EventType::WRAPPED_MIDI_EVENT:
        if (!_vst_midi_events_fifo.push(event))
        {
            MIND_LOG_WARNING("Vst wrapper, plugin: {}, MIDI queue Overflow!", name());
        }
        break;

    default:
        MIND_LOG_INFO("Vst wrapper, plugin: {}, received unhandled event", name());
        break;
    }

}

void Vst2xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypassed && !_can_do_soft_bypass)
    {
        bypass_process(in_buffer, out_buffer);
        _vst_midi_events_fifo.flush();
    }
    else
    {
        _vst_dispatcher(effProcessEvents, 0, 0, _vst_midi_events_fifo.flush(), 0.0f);
        _map_audio_buffers(in_buffer, out_buffer);
        _plugin_handle->processReplacing(_plugin_handle, _process_inputs, _process_outputs, AUDIO_CHUNK_SIZE);
    }
}

bool Vst2xWrapper::_update_speaker_arrangements(int inputs, int outputs)
{
    VstSpeakerArrangement in_arr;
    VstSpeakerArrangement out_arr;
    in_arr.numChannels = inputs;
    in_arr.type = arrangement_from_channels(inputs);
    out_arr.numChannels = outputs;
    out_arr.type = arrangement_from_channels(outputs);
    int res = _vst_dispatcher(effSetSpeakerArrangement, 0, (VstIntPtr)&in_arr, &out_arr, 0);
    return res == 1;
}

void Vst2xWrapper::_map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
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

void Vst2xWrapper::_update_mono_mode(bool speaker_arr_status)
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

VstSpeakerArrangementType arrangement_from_channels(int channels)
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
}

} // namespace vst2
} // namespace sushi