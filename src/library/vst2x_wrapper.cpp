#include "library/vst2x_wrapper.h"

#include "logging.h"

namespace {

static constexpr int VST_STRING_BUFFER_SIZE = 256;

} // anonymous namespace

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER;

ProcessorReturnCode Vst2xWrapper::init(const int sample_rate)
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

    // Channel setup
    set_input_channels(_plugin_handle->numInputs);
    set_output_channels(_plugin_handle->numOutputs);

    // Initialize internal plugin
    _vst_dispatcher(effOpen, 0, 0, 0, 0);
    _vst_dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(_sample_rate));
    _vst_dispatcher(effSetBlockSize, 0, AUDIO_CHUNK_SIZE, 0, 0);

    // Register internal parameters
    if (!_register_parameters())
    {
        _cleanup();
        return ProcessorReturnCode::PARAMETER_ERROR;
    }

    return ProcessorReturnCode::OK;
}

void Vst2xWrapper::_cleanup()
{
    if (_plugin_handle != nullptr)
    {
        // Tell plugin to stop and shutdown
        set_enabled(false);
        _vst_dispatcher(effClose, 0, 0, 0, 0);
    }
    if (_library_handle != nullptr)
    {
        PluginLoader::close_library_handle(_library_handle);
    }
}

bool Vst2xWrapper::_register_parameters()
{
    char param_name[VST_STRING_BUFFER_SIZE] = {0};
    char param_label[VST_STRING_BUFFER_SIZE] = {0};

    VstInt32 idx = 0;
    bool param_inserted_ok = true;
    while ( param_inserted_ok && (idx < _plugin_handle->numParams) )
    {
       _vst_dispatcher(effGetParamName, idx, 0, param_name, 0);
       _vst_dispatcher(effGetParamLabel, idx, 0, param_label, 0);
        // All VsT parameters are float 0..1
        auto pre_processor = new FloatParameterPreProcessor(0.0f, 1.0f);
        float default_value = _plugin_handle->getParameter(_plugin_handle, idx);
        FloatStompBoxParameter* param = new FloatStompBoxParameter(param_name, param_label, default_value, pre_processor);
        _parameters_by_index.push_back(param);
        param->set_id(static_cast<ObjectId>(idx));
        std::tie(std::ignore, param_inserted_ok) =
            _parameters.insert(std::pair<std::string, std::unique_ptr<BaseStompBoxParameter>>
                                       (param->name(), std::unique_ptr<BaseStompBoxParameter>(param)));
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

std::pair<ProcessorReturnCode, ObjectId> Vst2xWrapper::parameter_id_from_name(const std::string& parameter_name)
{
    auto parameter = get_parameter(parameter_name);
    if (parameter)
    {
        return std::make_pair(ProcessorReturnCode::OK, parameter->id());
    }
    return std::make_pair(ProcessorReturnCode::PARAMETER_NOT_FOUND, 0u);
}

void Vst2xWrapper::process_event(Event event)
{
    switch (event.type())
    {
    case EventType::FLOAT_PARAMETER_CHANGE:
    {
        auto typed_event = event.parameter_change_event();
        auto id = typed_event->param_id();
        assert(id < _parameters_by_index.size());
        auto parameter = static_cast<FloatStompBoxParameter*>(_parameters_by_index[id]);
        float value = static_cast<float>(typed_event->value());
        parameter->set(value);
        _plugin_handle->setParameter(_plugin_handle, static_cast<VstInt32>(id), value);
    }
    break;

    default:
        MIND_LOG_INFO("Vst wrapper, plugin: {}, received unhandled event", name());
        break;
    }

}

void Vst2xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    for (int i = 0; i < _current_input_channels; i++)
    {
        _process_inputs[i] = const_cast<float*>(in_buffer.channel(i));
    }
    for (int i = 0; i < _current_output_channels; i++)
    {
        _process_outputs[i] = out_buffer.channel(i);
    }

    _plugin_handle->processReplacing(_plugin_handle, _process_inputs, _process_outputs, AUDIO_CHUNK_SIZE);
}


} // namespace vst2
} // namespace sushi