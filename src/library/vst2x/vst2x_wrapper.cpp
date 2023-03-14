/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Wrapper for VST 2.x plugins.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "twine/twine.h"

#include "vst2x_wrapper.h"
#include "library/midi_decoder.h"
#include "logging.h"

namespace {

constexpr int VST_STRING_BUFFER_SIZE = 256;
constexpr int SINGLE_PROGRAM = 1;
char canDoBypass[] = "bypass";

} // anonymous namespace

namespace sushi {
namespace vst2 {

constexpr uint32_t SUSHI_HOST_TIME_CAPABILITIES = kVstNanosValid | kVstPpqPosValid | kVstTempoValid |
                                                  kVstBarsValid | kVstTimeSigValid;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("vst2");

Vst2xWrapper::~Vst2xWrapper()
{
    SUSHI_LOG_DEBUG("Unloading plugin {}", this->name());
    _cleanup();
}

ProcessorReturnCode Vst2xWrapper::init(float sample_rate)
{
    // TODO: sanity checks on sample_rate,
    //       but these can probably better be handled on Processor::init()
    _sample_rate = sample_rate;

    // Load shared library and VsT struct
    _library_handle = PluginLoader::get_library_handle_for_plugin(_host_control.to_absolute_path(_plugin_path));
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
    if (_plugin_handle->magic != kEffectMagic)
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
    int enabled = _vst_dispatcher(effCanDo, 0, 0, canDoBypass, 0);
    _can_do_soft_bypass = (enabled == 1);
    _number_of_programs = _plugin_handle->numPrograms;
    SUSHI_LOG_INFO_IF(enabled, "Plugin supports soft bypass");

    _has_binary_programs = _plugin_handle->flags &= effFlagsProgramChunks;

    // Channel setup
    _max_input_channels = _plugin_handle->numInputs;
    _max_output_channels = _plugin_handle->numOutputs;

    // Initialize internal plugin
    _vst_dispatcher(effOpen, 0, 0, nullptr, 0);
    _vst_dispatcher(effSetSampleRate, 0, 0, nullptr, _sample_rate);
    _vst_dispatcher(effSetBlockSize, 0, AUDIO_CHUNK_SIZE, nullptr, 0);

    // Register internal parameters
    if (!_register_parameters())
    {
        _cleanup();
        return ProcessorReturnCode::PARAMETER_ERROR;
    }

    // Register yourself
    _plugin_handle->user = this;
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
    _vst_dispatcher(effSetSampleRate, 0, 0, nullptr, _sample_rate);
    if (reset_enabled)
    {
        set_enabled(true);
    }
}

void Vst2xWrapper::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    [[maybe_unused]] bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    SUSHI_LOG_WARNING_IF(!valid_arr, "Failed to set a valid speaker arrangement")
}

void Vst2xWrapper::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    [[maybe_unused]] bool valid_arr = _update_speaker_arrangements(_current_input_channels, _current_output_channels);
    SUSHI_LOG_WARNING_IF(!valid_arr, "Failed to set a valid speaker arrangement")
}


void Vst2xWrapper::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
    if (enabled)
    {
        _vst_dispatcher(effMainsChanged, 0, 1, nullptr, 0.0f);
        _vst_dispatcher(effStartProcess, 0, 0, nullptr, 0.0f);
    }
    else
    {
        _vst_dispatcher(effMainsChanged, 0, 0, nullptr, 0.0f);
        _vst_dispatcher(effStopProcess, 0, 0, nullptr, 0.0f);
    }
}

void Vst2xWrapper::set_bypassed(bool bypassed)
{
    assert(twine::is_current_thread_realtime() == false);
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

std::pair<ProcessorReturnCode, float> Vst2xWrapper::parameter_value(ObjectId parameter_id) const
{
    if (static_cast<int>(parameter_id) < _plugin_handle->numParams)
    {
        float value  = _plugin_handle->getParameter(_plugin_handle, static_cast<VstInt32>(parameter_id));
        return {ProcessorReturnCode::OK, value};
    }

    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, 0.0f};
}

std::pair<ProcessorReturnCode, float> Vst2xWrapper::parameter_value_in_domain(ObjectId parameter_id) const
{
    return this->parameter_value(parameter_id);
}

std::pair<ProcessorReturnCode, std::string> Vst2xWrapper::parameter_value_formatted(ObjectId parameter_id) const
{
    if (static_cast<int>(parameter_id) < _plugin_handle->numParams)
    {
        // Many plugins don't respect the kVstMaxParamStrLen limit, so better use a larger buffer
        char buffer[VST_STRING_BUFFER_SIZE] = "";
        _vst_dispatcher(effGetParamDisplay, static_cast<VstInt32>(parameter_id), 0, buffer, 0);
        buffer[VST_STRING_BUFFER_SIZE-1] = 0;
        return {ProcessorReturnCode::OK, buffer};
    }
    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
}

int Vst2xWrapper::current_program() const
{
    if (this->supports_programs())
    {
        return _vst_dispatcher(effGetProgram, 0, 0, nullptr, 0);
    }
    return 0;
}

std::string Vst2xWrapper::current_program_name() const
{
    if (this->supports_programs())
    {
        char buffer[VST_STRING_BUFFER_SIZE] = "";
        _vst_dispatcher(effGetProgramName, 0, 0, buffer, 0);
        buffer[VST_STRING_BUFFER_SIZE-1] = 0;
        return {buffer};
    }
    return {};
}

std::pair<ProcessorReturnCode, std::string> Vst2xWrapper::program_name(int program) const
{
    if (this->supports_programs())
    {
        char buffer[VST_STRING_BUFFER_SIZE] = "";
        auto success = _vst_dispatcher(effGetProgramNameIndexed, program, 0, buffer, 0);
        buffer[VST_STRING_BUFFER_SIZE-1] = 0;
        return {success ? ProcessorReturnCode::OK : ProcessorReturnCode::PARAMETER_NOT_FOUND, buffer};
    }
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, ""};
}

std::pair<ProcessorReturnCode, std::vector<std::string>> Vst2xWrapper::all_program_names() const
{
    if (!this->supports_programs())
    {
        return {ProcessorReturnCode::UNSUPPORTED_OPERATION, std::vector<std::string>()};
    }
    std::vector<std::string> programs;
    for (int i = 0; i < _number_of_programs; ++i)
    {
        char buffer[VST_STRING_BUFFER_SIZE] = "";
        _vst_dispatcher(effGetProgramNameIndexed, i, 0, buffer, 0);
        buffer[VST_STRING_BUFFER_SIZE-1] = 0;
        programs.emplace_back(buffer);
    }
    return {ProcessorReturnCode::OK, programs};
}

ProcessorReturnCode Vst2xWrapper::set_program(int program)
{
    if (this->supports_programs() && program < _number_of_programs)
    {
        _vst_dispatcher(effBeginSetProgram, 0, 0, nullptr, 0);
        /* Vst2 lacks a mechanism for signaling that the program change was successful */
        _vst_dispatcher(effSetProgram, 0, program, nullptr, 0);
        _vst_dispatcher(effEndSetProgram, 0, 0, nullptr, 0);
        _host_control.post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED,
                                                                 this->id(), 0, IMMEDIATE_PROCESS));
        return ProcessorReturnCode::OK;
    }
    return ProcessorReturnCode::UNSUPPORTED_OPERATION;
}

void Vst2xWrapper::_cleanup()
{
    if (_plugin_handle != nullptr)
    {
        // Tell plugin to stop and shutdown
        set_enabled(false);
        _vst_dispatcher(effClose, 0, 0, nullptr, 0);
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
    char param_unit[VST_STRING_BUFFER_SIZE] = {0};

    VstInt32 idx = 0;
    bool param_inserted_ok = true;
    while (param_inserted_ok && (idx < _plugin_handle->numParams) )
    {
        _vst_dispatcher(effGetParamName, idx, 0, param_name, 0);
        _vst_dispatcher(effGetParamLabel, idx, 0, param_unit, 0);
        param_name[VST_STRING_BUFFER_SIZE-1] = 0;
        param_unit[VST_STRING_BUFFER_SIZE-1] = 0;
        auto unique_name = _make_unique_parameter_name(param_name);

        param_inserted_ok = register_parameter(new FloatParameterDescriptor(unique_name,
                                                                            param_name,
                                                                            param_unit,
                                                                            0,
                                                                            1,
                                                                            Direction::AUTOMATABLE,
                                                                            nullptr));
        if (param_inserted_ok)
        {
            SUSHI_LOG_DEBUG("Plugin: {}, registered param: {}", name(), param_name);
        }
        else
        {
            SUSHI_LOG_ERROR("Plugin: {}, Error while registering param: {}", name(), param_name);
        }
        idx++;
    }

    return param_inserted_ok;
}

void Vst2xWrapper::process_event(const RtEvent& event)
{

    switch(event.type())
    {
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        {
            auto typed_event = event.parameter_change_event();
            auto id = typed_event->param_id();
            assert(static_cast<VstInt32>(id) < _plugin_handle->numParams);
            _plugin_handle->setParameter(_plugin_handle, static_cast<VstInt32>(id), typed_event->value());
            break;
        }

        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::PITCH_BEND:
        case RtEventType::AFTERTOUCH:
        case RtEventType::MODULATION:
        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            if (_vst_midi_events_fifo.push(event) == false)
            {
                SUSHI_LOG_WARNING("Plugin: {}, MIDI queue Overflow!", name());
            }
            break;
        }

        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _set_bypass_rt(bypassed);
            break;
        }

        case RtEventType::SET_STATE:
        {
            auto state = event.processor_state_event()->state();
            _set_state_rt(state);
            break;
        }

        default:
            break;
    }
}

void Vst2xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_can_do_soft_bypass == false && _bypass_manager.should_process() == false)
    {
        bypass_process(in_buffer, out_buffer);
        _vst_midi_events_fifo.flush();
    }
    else
    {
        _vst_dispatcher(effProcessEvents, 0, 0, _vst_midi_events_fifo.flush(), 0.0f);
        _map_audio_buffers(in_buffer, out_buffer);
        _plugin_handle->processReplacing(_plugin_handle, _process_inputs, _process_outputs, AUDIO_CHUNK_SIZE);
        if (_can_do_soft_bypass == false && _bypass_manager.should_ramp())
        {
            _bypass_manager.crossfade_output(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        }
    }
}

void Vst2xWrapper::notify_parameter_change_rt(VstInt32 parameter_index, float value)
{
    /* The default Vst2.4 implementation calls set_parameter() in set_parameter_automated()
     * so the plugin is already aware of the change, we just need to send a notification
     * to the non-rt part */
    if (parameter_index > this->parameter_count())
    {
        return;
    }
    if (maybe_output_cv_value(parameter_index, value) == false)
    {
        auto e = RtEvent::make_parameter_change_event(this->id(), 0, static_cast<ObjectId>(parameter_index), value);
        output_event(e);
    }
}

void Vst2xWrapper::notify_parameter_change(VstInt32 parameter_index, float value)
{
    auto e = new ParameterChangeNotificationEvent(this->id(),
                                                  static_cast<ObjectId>(parameter_index),
                                                  value,
                                                  value,
                                                  this->parameter_value_formatted(parameter_index).second,
                                                  IMMEDIATE_PROCESS);
    _host_control.post_event(e);
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

VstTimeInfo* Vst2xWrapper::time_info()
{
    auto transport = _host_control.transport();
    auto ts = transport->time_signature();

    _time_info.samplePos          = transport->current_samples();
    _time_info.sampleRate         = _sample_rate;
    _time_info.nanoSeconds        = std::chrono::duration_cast<std::chrono::nanoseconds>(transport->current_process_time()).count();
    _time_info.ppqPos             = transport->current_beats();
    _time_info.tempo              = transport->current_tempo();
    _time_info.barStartPos        = transport->current_bar_start_beats();
    _time_info.timeSigNumerator   = ts.numerator;
    _time_info.timeSigDenominator = ts.denominator;
    _time_info.flags = (SUSHI_HOST_TIME_CAPABILITIES | transport->playing()) ? kVstTransportPlaying : 0;
    if (transport->current_state_change() != PlayStateChange::UNCHANGED)
    {
        _time_info.flags |= kVstTransportChanged;
    }
    return &_time_info;
}

void Vst2xWrapper::_map_audio_buffers(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    int i;

    for (i = 0; i < _current_input_channels; ++i)
    {
        _process_inputs[i] = const_cast<float*>(in_buffer.channel(i));
    }
    for (; i <= _max_input_channels; ++i)
    {
        _process_inputs[i] = (_dummy_input.channel(0));
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

void Vst2xWrapper::output_vst_event(const VstEvent* event)
{
    assert(event);
    if (event->type == kVstMidiType)
    {
        auto midi_event = reinterpret_cast<const VstMidiEvent*>(event);
        MidiDataByte midi_data = midi::to_midi_data_byte(reinterpret_cast<const uint8_t*>(midi_event->midiData), 3);
        output_midi_event_as_internal(midi_data, event->deltaFrames);
    }
}

ProcessorReturnCode Vst2xWrapper::set_state(ProcessorState* state, bool realtime_running)
{
    if (state->program().has_value())
    {
        this->set_program(state->program().value());
    }

    if (realtime_running)
    {
        auto rt_state = std::make_unique<RtState>(*state);
        auto event = new RtStateEvent(this->id(), std::move(rt_state), IMMEDIATE_PROCESS);
        _host_control.post_event(event);
    }

    else
    {
        if (state->bypassed().has_value())
        {
            _set_bypass_rt(state->bypassed().value());
        }

        for (const auto& parameter : state->parameters())
        {
            VstInt32 id = parameter.first;
            float value = parameter.second;
            if (id < _plugin_handle->numParams)
            {
                _plugin_handle->setParameter(_plugin_handle, id, value);
            }
        }
    }
    return ProcessorReturnCode::OK;
}

ProcessorState Vst2xWrapper::save_state() const
{
    ProcessorState state;
    if (_has_binary_programs)
    {
        std::byte* data = nullptr;
        int size = _vst_dispatcher(effGetChunk, SINGLE_PROGRAM, reinterpret_cast<VstIntPtr>(&data), nullptr, 0);
        if (size > 0)
        {
            state.set_binary_data(std::vector<std::byte>(data, data + size));
        }
        if (_can_do_soft_bypass == false)
        {
            state.set_bypass(bypassed());
        }
    }
    else
    {
        for (int id = 0; id < _plugin_handle->numParams; ++id)
        {
            float value = _plugin_handle->getParameter(_plugin_handle, static_cast<VstInt32>(id));
            state.add_parameter_change(id, value);
        }
        state.set_bypass(bypassed());
    }
    return state;
}

PluginInfo Vst2xWrapper::info() const
{
    PluginInfo info;
    info.type = PluginType::VST2X;
    info.path = _plugin_path;
    return info;
}

void Vst2xWrapper::_set_bypass_rt(bool bypassed)
{
    _bypass_manager.set_bypass(bypassed, _sample_rate);
    if (_can_do_soft_bypass)
    {
        _vst_dispatcher(effSetBypass, 0, bypassed ? 1 : 0, nullptr, 0.0f);
    }
}

void Vst2xWrapper::_set_state_rt(RtState* state)
{
    if (state->bypassed().has_value())
    {
        _set_bypass_rt(*state->bypassed());
    }

    for (const auto& parameter : state->parameters())
    {
        VstInt32 id = parameter.first;
        float value = parameter.second;
        if (id < _plugin_handle->numParams)
        {
            _plugin_handle->setParameter(_plugin_handle, id, value);
        }
    }
    async_delete(state);
    notify_state_change_rt();
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
            return kSpeakerArr80Music;
    }
}

} // namespace vst2
} // namespace sushi
