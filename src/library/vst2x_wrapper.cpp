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

    // TODO: derive parameter instantiation code from something like this in minihost.cpp
    // for (VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++)
    // {
    //     char paramName[256] = {0};
    //     char paramLabel[256] = {0};
    //     char paramDisplay[256] = {0};

    //     effect->dispatcher (effect, effGetParamName, paramIndex, 0, paramName, 0);
    //     effect->dispatcher (effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
    //     effect->dispatcher (effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
    //     float value = effect->getParameter (effect, paramIndex);

    //     printf ("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName, paramDisplay, paramLabel, value);
    //  }

    // Initialize internal plugin
    _vst_dispatcher(effOpen, 0, 0, 0, 0);
    _vst_dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(_sample_rate));
    _vst_dispatcher(effSetBlockSize, 0, AUDIO_CHUNK_SIZE, 0, 0);

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

void Vst2xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    for (int i=0; i<_current_input_channels; i++)
    {
        _process_inputs[i] = const_cast<float*>(in_buffer.channel(i));
    }
    for (int i=0; i<_current_output_channels; i++)
    {
        _process_outputs[i] = out_buffer.channel(i);
    }

    _plugin_handle->processReplacing(_plugin_handle, _process_inputs, _process_outputs, AUDIO_CHUNK_SIZE);
}

void Vst2xWrapper::process_event(Event /*event*/)
{
    // TODO: eventually get VSTparam id from internal map
    // and  call dispatcher->(setEffect ...)

}

} // namespace vst2
} // namespace sushi