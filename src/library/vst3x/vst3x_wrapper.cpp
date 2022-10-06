/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Wrapper for VST 3.x plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <string>
#include <cstdlib>
#include <dirent.h>
#include <unistd.h>

#include <pluginterfaces/base/ustring.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <public.sdk/source/vst/vstpresetfile.h>
#include <public.sdk/source/common/memorystream.h>

#include <twine/twine.h>

#include "vst3x_wrapper.h"
#include "library/event.h"
#include "logging.h"

namespace sushi {
namespace vst3 {

constexpr int VST_NAME_BUFFER_SIZE = 128;

constexpr char VST_PRESET_SUFFIX[] = ".vstpreset";
constexpr int VST_PRESET_SUFFIX_LENGTH = 10;

constexpr uint32_t SUSHI_HOST_TIME_CAPABILITIES = Steinberg::Vst::ProcessContext::kSystemTimeValid &
                                                  Steinberg::Vst::ProcessContext::kContTimeValid &
                                                  Steinberg::Vst::ProcessContext::kBarPositionValid &
                                                  Steinberg::Vst::ProcessContext::kTempoValid &
                                                  Steinberg::Vst::ProcessContext::kTimeSigValid;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("vst3");

// Convert a Steinberg 128 char unicode string to 8 bit ascii std::string
std::string to_ascii_str(Steinberg::Vst::String128 wchar_buffer)
{
    char char_buf[128] = {};
    Steinberg::UString128 str(wchar_buffer, 128);
    str.toAscii(char_buf, VST_NAME_BUFFER_SIZE);
    return {char_buf};
}

// Get all vst3 preset locations in the right priority order
// See Steinberg documentation of "Preset locations".
std::vector<std::string> get_preset_locations()
{
    std::vector<std::string> locations;
    char* home_dir = getenv("HOME");
    if (home_dir != nullptr)
    {
        locations.push_back(std::string(home_dir) + "/.vst3/presets/");
    }
    SUSHI_LOG_WARNING_IF(home_dir == nullptr, "Failed to get home directory")
    locations.emplace_back("/usr/share/vst3/presets/");
    locations.emplace_back("/usr/local/share/vst3/presets/");
    char buffer[_POSIX_SYMLINK_MAX + 1] = {0};
    auto path_length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (path_length > 0)
    {
        std::string path(buffer);
        auto pos = path.find_last_of('/');
        if (pos != std::string::npos)
        {
            locations.emplace_back(path.substr(0, pos) + "/vst3/presets/");
        }
        else
        {
            path_length = 0;
        }
    }
    SUSHI_LOG_WARNING_IF(path_length <= 0, "Failed to get binary directory")
    return locations;
}

std::string extract_preset_name(const std::string& path)
{
    auto fname_pos = path.find_last_of('/') + 1;
    return path.substr(fname_pos, path.length() - fname_pos - VST_PRESET_SUFFIX_LENGTH);
}

// Recursively search subdirs for preset files
void add_patches(const std::string& path, std::vector<std::string>& patches)
{
    SUSHI_LOG_INFO("Looking for presets in: {}", path);
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
    {
        return;
    }
    dirent* entry;
    while((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_type == DT_REG)
        {
            std::string patch_name(entry->d_name);
            auto suffix_pos = patch_name.rfind(VST_PRESET_SUFFIX);
            if (suffix_pos != std::string::npos && patch_name.length() - suffix_pos == VST_PRESET_SUFFIX_LENGTH)
            {
                SUSHI_LOG_DEBUG("Reading vst preset patch: {}", patch_name);
                patches.push_back(path + "/" + patch_name);
            }
        }
        else if (entry->d_type == DT_DIR && entry->d_name[0] != '.') /* Dirty way to ignore ./,../ and hidden files */
        {
            add_patches(path + "/" +  entry->d_name, patches);
        }
    }
    closedir(dir);
}

std::vector<std::string> enumerate_patches(const std::string& plugin_name, const std::string& company)
{
    /* VST3 standard says you should put preset files in specific locations, So we recursively
     * scan these folders for all files that match, just like we do with Re plugins*/
    std::vector<std::string> patches;
    std::vector<std::string> paths = get_preset_locations();
    for (const auto& path : paths)
    {
        add_patches(path + company + "/" + plugin_name, patches);
    }
    return patches;
}

void Vst3xWrapper::_cleanup()
{
    if (_instance.component())
    {
        set_enabled(false);
    }
    if (_state_parameter_changes)
    {
        delete _state_parameter_changes;
    }
}

Vst3xWrapper::~Vst3xWrapper()
{
    SUSHI_LOG_DEBUG("Unloading plugin {}", this->name());
    _cleanup();
}

ProcessorReturnCode Vst3xWrapper::init(float sample_rate)
{
    _sample_rate = sample_rate;
    bool loaded = _instance.load_plugin(_host_control.to_absolute_path(_plugin_load_path), _plugin_load_name);
    if (!loaded)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_LOAD_ERROR;
    }
    set_name(_instance.name());
    set_label(_instance.name());

    if (!_setup_audio_buses() || !_setup_event_buses())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    auto res = _instance.component()->setActive(Steinberg::TBool(true));
    if (res != Steinberg::kResultOk)
    {
        SUSHI_LOG_ERROR("Failed to activate component with error code: {}", res);
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    res = _instance.controller()->setComponentHandler(&_component_handler);
    if (res != Steinberg::kResultOk)
    {
        SUSHI_LOG_ERROR("Failed to set component handler with error code: {}", res);
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    if (!_sync_processor_to_controller())
    {
        SUSHI_LOG_WARNING("failed to sync controller");
    }

    if (!_setup_processing())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    if (!_register_parameters())
    {
        return ProcessorReturnCode::PARAMETER_ERROR;
    }
    if (!_setup_internal_program_handling())
    {
        _setup_file_program_handling();
    }
    return ProcessorReturnCode::OK;
}

void Vst3xWrapper::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    bool reset_enabled = enabled();
    if (reset_enabled)
    {
        set_enabled(false);
    }
    if (!_setup_processing())
    {
        // TODO how to handle this?
        SUSHI_LOG_ERROR("Error setting sample rate to {}", sample_rate);
    }
    if (reset_enabled)
    {
        set_enabled(true);
    }
}

void Vst3xWrapper::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        {
            auto typed_event = event.parameter_change_event();
            _add_parameter_change(typed_event->param_id(), typed_event->value(), typed_event->sample_offset());
            _parameter_update_queue.push({typed_event->param_id(), typed_event->value()});
            _notify_parameter_change = true;
            break;
        }
        case RtEventType::NOTE_ON:
        {
            auto vst_event = convert_note_on_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case RtEventType::NOTE_OFF:
        {
            auto vst_event = convert_note_off_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case RtEventType::NOTE_AFTERTOUCH:
        {
            auto vst_event = convert_aftertouch_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case RtEventType::MODULATION:
        {
            if (_mod_wheel_parameter.supported)
            {
                auto typed_event = event.keyboard_common_event();
                _add_parameter_change(_mod_wheel_parameter.id, typed_event->value(), typed_event->sample_offset());
            }
            break;
        }
        case RtEventType::PITCH_BEND:
        {
            if (_pitch_bend_parameter.supported)
            {
                auto typed_event = event.keyboard_common_event();
                float pb_value = (typed_event->value() + 1.0f) * 0.5f;
                _add_parameter_change(_pitch_bend_parameter.id, pb_value, typed_event->sample_offset());
            }
            break;
        }
        case RtEventType::AFTERTOUCH:
        {
            if (_aftertouch_parameter.supported)
            {
                auto typed_event = event.keyboard_common_event();
                _add_parameter_change(_aftertouch_parameter.id, typed_event->value(), typed_event->sample_offset());
            }
            break;
        }
        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _bypass_manager.set_bypass(bypassed, _sample_rate);
            break;
        }
        case RtEventType::SET_STATE:
        {
            auto state = event.processor_state_event()->state();
            _set_state_rt(static_cast<Vst3xRtState*>(state));
            break;
        }
        default:
            break;
    }
}

void Vst3xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypass_parameter.supported == false && _bypass_manager.should_process() == false)
    {
        bypass_process(in_buffer, out_buffer);
    }
    else
    {
        _fill_processing_context();
        if (_state_parameter_changes)
        {
            _process_data.inputParameterChanges = _state_parameter_changes;
        }
        _process_data.assign_buffers(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        _instance.processor()->process(_process_data);
        if (_bypass_parameter.supported == false && _bypass_manager.should_ramp())
        {
            _bypass_manager.crossfade_output(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        }
        _forward_events(_process_data);
        _forward_params(_process_data);
    }

    if (_notify_parameter_change)
    {
        request_non_rt_task(parameter_update_callback);
        _notify_parameter_change = false;
    }

    if (_state_parameter_changes)
    {
        _process_data.inputParameterChanges = &_in_parameter_changes;
        async_delete(_state_parameter_changes);
        _state_parameter_changes = nullptr;
    }
    _process_data.clear();
}

void Vst3xWrapper::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    _setup_channels();
}

void Vst3xWrapper::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    _setup_channels();
}


void Vst3xWrapper::set_enabled(bool enabled)
{
    if (enabled == _enabled)
    {
        return;
    }

    // Activate component first, then enable processing, but deactivate in reverse order
    // See: https://developer.steinberg.help/display/VST/Audio+Processor+Call+Sequence
    if (enabled)
    {
        _instance.component()->setActive(true);
        _instance.processor()->setProcessing(true);
    }
    else
    {
        _instance.processor()->setProcessing(false);
        _instance.component()->setActive(false);
    }
    Processor::set_enabled(enabled);
}

void Vst3xWrapper::set_bypassed(bool bypassed)
{
    assert(twine::is_current_thread_realtime() == false);
    if (_bypass_parameter.supported)
    {
        _host_control.post_event(new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                                          this->id(), _bypass_parameter.id, bypassed? 1.0f : 0, IMMEDIATE_PROCESS));
        _bypass_manager.set_bypass(bypassed, _sample_rate);
    }
    else
    {
        _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
    }
}

bool Vst3xWrapper::bypassed() const
{
    if (_bypass_parameter.supported)
    {
        float value;
        std::tie(std::ignore, value) = this->parameter_value(_bypass_parameter.id);
        return value > 0.5;
    }
    return _bypass_manager.bypassed();
}

const ParameterDescriptor* Vst3xWrapper::parameter_from_id(ObjectId id) const
{
    auto descriptor = _parameters_by_vst3_id.find(static_cast<Steinberg::Vst::ParamID>(id));
    if (descriptor !=  _parameters_by_vst3_id.end())
    {
        return descriptor->second;
    }
    return nullptr;
}

std::pair<ProcessorReturnCode, float> Vst3xWrapper::parameter_value(ObjectId parameter_id) const
{
    /* Always returns OK as the default vst3 implementation just returns 0 for invalid parameter ids */
    auto controller = const_cast<PluginInstance*>(&_instance)->controller();
    auto value = controller->getParamNormalized(parameter_id);
    return {ProcessorReturnCode::OK, static_cast<float>(value)};
}

std::pair<ProcessorReturnCode, float> Vst3xWrapper::parameter_value_in_domain(ObjectId parameter_id) const
{
    /* Always returns OK as the default vst3 implementation just returns 0 for invalid parameter ids */
    auto controller = const_cast<PluginInstance*>(&_instance)->controller();
    auto value = controller->normalizedParamToPlain(parameter_id, controller->getParamNormalized(parameter_id));
    return {ProcessorReturnCode::OK, static_cast<float>(value)};
}

std::pair<ProcessorReturnCode, std::string> Vst3xWrapper::parameter_value_formatted(ObjectId parameter_id) const
{
    auto controller = const_cast<PluginInstance*>(&_instance)->controller();
    auto value = controller->getParamNormalized(parameter_id);
    Steinberg::Vst::String128 buffer = {};
    auto res = controller->getParamStringByValue(parameter_id, value, buffer);
    if (res == Steinberg::kResultOk)
    {
        return {ProcessorReturnCode::OK, to_ascii_str(buffer)};
    }
    return {ProcessorReturnCode::PARAMETER_NOT_FOUND, ""};
}

int Vst3xWrapper::current_program() const
{
    if (_supports_programs)
    {
        return _current_program;
    }
    return 0;
}

std::string Vst3xWrapper::current_program_name() const
{
    return program_name(_current_program).second;
}

std::pair<ProcessorReturnCode, std::string> Vst3xWrapper::program_name(int program) const
{
    if (_supports_programs && _internal_programs)
    {
        SUSHI_LOG_INFO("Program name {}", program);
        auto mutable_unit = const_cast<PluginInstance*>(&_instance)->unit_info();
        Steinberg::Vst::String128 buffer;
        auto res = mutable_unit->getProgramName(_main_program_list_id, program, buffer);
        if (res == Steinberg::kResultOk)
        {
            SUSHI_LOG_INFO("Program name returned error {}", res);
            return {ProcessorReturnCode::OK, to_ascii_str(buffer)};
        }
    }
    else if (_supports_programs && _file_based_programs && program < static_cast<int>(_program_files.size()))
    {
        return {ProcessorReturnCode::OK, extract_preset_name(_program_files[program])};
    }
    SUSHI_LOG_INFO("Set program name failed");
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, ""};
}

std::pair<ProcessorReturnCode, std::vector<std::string>> Vst3xWrapper::all_program_names() const
{
    if (_supports_programs)
    {
        SUSHI_LOG_INFO("all Program names");
        std::vector<std::string> programs;
        auto mutable_unit = const_cast<PluginInstance*>(&_instance)->unit_info();
        for (int i = 0; i < _program_count; ++i)
        {
            if (_internal_programs)
            {
                Steinberg::Vst::String128 buffer;
                auto res = mutable_unit->getProgramName(_main_program_list_id, i, buffer);
                if (res == Steinberg::kResultOk)
                {
                    programs.push_back(to_ascii_str(buffer));
                } else
                {
                    SUSHI_LOG_INFO("Program name returned error {} on {}", res, i);
                    break;
                }
            }
            else if (_file_based_programs)
            {
                programs.push_back(extract_preset_name(_program_files[i]));
            }
        }
        SUSHI_LOG_INFO("Return list with {} programs", programs.size());
        return {ProcessorReturnCode::OK, programs};
    }
    SUSHI_LOG_INFO("All program names failed");
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, std::vector<std::string>()};
}

ProcessorReturnCode Vst3xWrapper::set_program(int program)
{
    if (!_supports_programs || _program_count == 0)
    {
        return ProcessorReturnCode::UNSUPPORTED_OPERATION;
    }
    if (_internal_programs)
    {
        float normalised_program_id = static_cast<float>(program) / static_cast<float>(_program_count);
        auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                              this->id(),
                                              _program_change_parameter.id,
                                              normalised_program_id,
                                              IMMEDIATE_PROCESS);
        event->set_completion_cb(Vst3xWrapper::program_change_callback, this);
        _host_control.post_event(event);
        SUSHI_LOG_INFO("Set program {}, {}, {}", program, normalised_program_id, _program_change_parameter.id);

        // TODO: Why is this commented out?
        //_instance.controller()->setParamNormalized(_program_change_parameter.id, normalised_program_id);

        return ProcessorReturnCode::OK;
    }
    else if (_file_based_programs && program < static_cast<int>(_program_files.size()))
    {
        SUSHI_LOG_INFO("Loading file based preset");
        Steinberg::OPtr<Steinberg::IBStream> stream(Steinberg::Vst::FileStream::open(_program_files[program].c_str(), "rb"));
        if (stream == nullptr)
        {
            SUSHI_LOG_INFO("Failed to load file {}", _program_files[program]);
            return ProcessorReturnCode::ERROR;
        }
        Steinberg::Vst::PresetFile preset_file(stream);
        preset_file.readChunkList();

        bool res = preset_file.restoreControllerState(_instance.controller());
        res &= preset_file.restoreComponentState(_instance.component());
        // Notify the processor of the update with an idle message. This was specific
        // to Retrologue and not part of the Vst3 standard, so we might remove it eventually
        Steinberg::Vst::HostMessage message;
        message.setMessageID("idle");
        if (_instance.notify_processor(&message) == false)
        {
            SUSHI_LOG_ERROR("Idle message returned error");
        }
        if (res)
        {
            _current_program = program;
            return ProcessorReturnCode::OK;
        } else
        {
            SUSHI_LOG_INFO("restore state returned error");
        }
    }
    SUSHI_LOG_INFO("Error in program change");
    return ProcessorReturnCode::ERROR;
}

ProcessorReturnCode Vst3xWrapper::set_state(ProcessorState* state, bool realtime_running)
{
    if (state->has_binary_data())
    {
        _set_binary_state(state->binary_data());
        return ProcessorReturnCode::OK;
    }

    std::unique_ptr<Vst3xRtState> rt_state;
    if (realtime_running)
    {
        rt_state = std::make_unique<Vst3xRtState>(*state);
    }

    if (state->program().has_value())
    {
        _set_program_state(*state->program(), rt_state.get(), realtime_running);
    }

    if (state->bypassed().has_value())
    {
        _set_bypass_state(*state->bypassed(), rt_state.get(), realtime_running);
    }

    for (const auto& parameter : state->parameters())
    {
        int id = parameter.first;
        float value = parameter.second;
        _instance.controller()->setParamNormalized(id, value);
        if (realtime_running == false)
        {
            _add_parameter_change(id, value, 0);
        }
    }

    if (realtime_running)
    {
        auto event = new RtStateEvent(this->id(), std::move(rt_state), IMMEDIATE_PROCESS);
        _host_control.post_event(event);
    }
    else
    {
        _host_control.post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED,
                                                                 this->id(), 0, IMMEDIATE_PROCESS));
    }

    return ProcessorReturnCode::OK;
}


ProcessorState Vst3xWrapper::save_state() const
{
    ProcessorState state;
    Steinberg::MemoryStream stream;
    if (_instance.component()->getState (&stream) == Steinberg::kResultTrue)
    {
        auto data_size = stream.getSize();
        auto data = reinterpret_cast<std::byte*>(stream.getData());
        state.set_binary_data(std::vector<std::byte>(data, data + data_size));
    }
    else
    {
        SUSHI_LOG_WARNING("Failed to get component state");
    }
    return state;
}

PluginInfo Vst3xWrapper::info() const
{
    PluginInfo info;
    info.type = PluginType::VST3X;
    info.uid  = _plugin_load_name;
    info.path = _plugin_load_path;
    return info;
}

bool Vst3xWrapper::_register_parameters()
{
    int param_count = _instance.controller()->getParameterCount();
    _in_parameter_changes.setMaxParameters(param_count);
    _out_parameter_changes.setMaxParameters(param_count);

    for (int i = 0; i < param_count; ++i)
    {
        Steinberg::Vst::ParameterInfo info;
        auto res = _instance.controller()->getParameterInfo(i, info);
        if (res == Steinberg::kResultOk)
        {
            /* Vst3 uses a model where parameters are indexed by an integer from 0 to
             * getParameterCount() - 1 (just like Vst2.4). But in addition, each parameter
             * also has a 32-bit integer id which is arbitrarily assigned.
             *
             * When doing real time parameter updates, the parameters must be accessed using this
             * id and not its index. Hence, the id in the registered ParameterDescriptors
             * store this id and not the index in the processor array like it does for the Vst2
             * wrapper and internal plugins. Hopefully that doesn't cause any issues. */
            auto param_name = to_ascii_str(info.title);
            auto param_unit = to_ascii_str(info.units);
            bool automatable_bool = info.flags & Steinberg::Vst::ParameterInfo::kCanAutomate;

            auto direction = automatable_bool ? Direction::AUTOMATABLE : Direction::OUTPUT;

            if (info.flags & Steinberg::Vst::ParameterInfo::kIsBypass)
            {
                _bypass_parameter.id = info.id;
                _bypass_parameter.supported = true;
                SUSHI_LOG_INFO("Plugin supports soft bypass");
            }
            else if (info.flags & Steinberg::Vst::ParameterInfo::kIsProgramChange &&
                     _program_change_parameter.supported == false)
            {
                /* For now, we only support 1 program change parameter, and we're counting on the
                 * first one to be the global one. Multitimbral instruments can have multiple
                 * program change parameters, but we'll have to look into how to support that. */
                _program_change_parameter.id = info.id;
                _program_change_parameter.supported = true;
                SUSHI_LOG_INFO("We have a program change parameter at {}", info.id);
            }
            else if (info.stepCount > 0 &&
                     register_parameter(new IntParameterDescriptor(_make_unique_parameter_name(param_name),
                                                                   param_name,
                                                                   param_unit,
                                                                   0,
                                                                   info.stepCount,
                                                                   direction,
                                                                   nullptr),
                                        info.id))
            {
                SUSHI_LOG_INFO("Registered INT parameter {}, id {}", param_name, info.id);
            }
            else if (register_parameter(new FloatParameterDescriptor(_make_unique_parameter_name(param_name),
                                                                     param_name,
                                                                     param_unit,
                                                                     0,
                                                                     1,
                                                                     direction,
                                                                     nullptr),
                                        info.id))
            {
                SUSHI_LOG_INFO("Registered parameter {}, id {}", param_name, info.id);
            }
            else
            {
                SUSHI_LOG_INFO("Error registering parameter {}.", param_name);
            }
        }
    }

    // Create a "backwards map" from Vst3 parameter ids to parameter indices
    for (auto param : this->all_parameters())
    {
        _parameters_by_vst3_id[param->id()] = param;
    }

    /* Steinberg decided not support standard midi, nor provide special events for common
     * controller (Pitch bend, mod wheel, etc.) instead these are exposed as regular
     * parameters, and we can query the plugin for what 'default' midi cc:s these parameters
     * would be mapped to if the plugin was able to handle native midi.
     * So we query the plugin for this and if that's the case, store the id:s of these
     * 'special' parameters, so we can map PB and Mod events to them.
     * Currently, we don't hide these parameters, unlike the bypass parameter, so they can
     * still be controlled via OSC or other controllers. */
    if (_instance.midi_mapper())
    {
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kCtrlModWheel,
                                                     _mod_wheel_parameter.id) == Steinberg::kResultOk)
        {
            SUSHI_LOG_INFO("Plugin supports mod wheel parameter mapping");
            _mod_wheel_parameter.supported = true;
        }
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kPitchBend,
                                                     _pitch_bend_parameter.id) == Steinberg::kResultOk)
        {
            SUSHI_LOG_INFO("Plugin supports pitch bend parameter mapping");
            _pitch_bend_parameter.supported = true;
        }
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kAfterTouch,
                                                     _aftertouch_parameter.id) == Steinberg::kResultOk)
        {
            SUSHI_LOG_INFO("Plugin supports aftertouch parameter mapping");
            _aftertouch_parameter.supported = true;
        }
    }

    return true;
}

bool Vst3xWrapper::_setup_audio_buses()
{
    int input_audio_buses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput);
    int output_audio_buses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput);
    SUSHI_LOG_INFO("Plugin has {} audio input buffers and {} audio output buffers", input_audio_buses, output_audio_buses);
    if (output_audio_buses == 0)
    {
        return false;
    }
    _max_input_channels = 0;
    _max_output_channels = 0;
    /* Setup 1 main output bus and 1 main input bus (if available) */
    Steinberg::Vst::BusInfo info;
    for (int i = 0; i < input_audio_buses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_input_channels = info.channelCount;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                SUSHI_LOG_ERROR("Failed to activate plugin input bus {}", i);
                return false;
            }
            break;
        }
    }
    for (int i = 0; i < output_audio_buses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_output_channels = info.channelCount;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                SUSHI_LOG_ERROR("Failed to activate plugin output bus {}", i);
                return false;
            }
            break;
        }
    }
    SUSHI_LOG_INFO("Vst3 wrapper ({}) has {} inputs and {} outputs", this->name(), _max_input_channels, _max_output_channels);
    return true;
}

bool Vst3xWrapper::_setup_event_buses()
{
    int input_buses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kInput);
    int output_buses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kOutput);
    SUSHI_LOG_INFO("Plugin has {} event input buffers and {} event output buffers", input_buses, output_buses);
    /* Try to activate all buses here */
    for (int i = 0; i < input_buses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            SUSHI_LOG_ERROR("Failed to activate plugin input event bus {}", i);
            return false;
        }
    }
    for (int i = 0; i < output_buses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                      Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            SUSHI_LOG_ERROR("Failed to activate plugin output event bus {}", i);
            return false;
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_channels()
{
    SUSHI_LOG_INFO("Vst3 wrapper ({}) setting up {} inputs and {} outputs", this->name(), _current_input_channels, _current_output_channels);
    Steinberg::Vst::SpeakerArrangement input_arr = speaker_arr_from_channels(_current_input_channels);
    Steinberg::Vst::SpeakerArrangement output_arr = speaker_arr_from_channels(_current_output_channels);

    /* numIns and numOuts refer to the number of buses, not channels, the docs are very vague on this point */
    auto res = _instance.processor()->setBusArrangements(&input_arr, (_max_input_channels == 0)? 0:1, &output_arr, 1);
    if (res != Steinberg::kResultOk)
    {
        SUSHI_LOG_ERROR("Failed to set a valid channel arrangement");
        return false;
    }
    return true;
}

bool Vst3xWrapper::_setup_processing()
{
    _process_data.processContext->sampleRate = _sample_rate;
    Steinberg::Vst::ProcessSetup setup;
    setup.maxSamplesPerBlock = AUDIO_CHUNK_SIZE;
    setup.processMode = Steinberg::Vst::ProcessModes::kRealtime;
    setup.sampleRate = _sample_rate;
    setup.symbolicSampleSize = Steinberg::Vst::SymbolicSampleSizes::kSample32;
    auto res = _instance.processor()->setupProcessing(setup);
    if (res != Steinberg::kResultOk)
    {
        SUSHI_LOG_ERROR("Error setting up processing, error code: {}", res);
        return false;
    }
    return true;
}

bool Vst3xWrapper::_setup_internal_program_handling()
{
    if (_instance.unit_info() == nullptr || _program_change_parameter.supported == false)
    {
        SUSHI_LOG_INFO("No unit info or program change parameter");
        return false;
    }
    if (_instance.unit_info()->getProgramListCount() == 0)
    {
        SUSHI_LOG_INFO("ProgramListCount is 0");
        return false;
    }
    _main_program_list_id = 0;
    Steinberg::Vst::UnitInfo info;
    auto res = _instance.unit_info()->getUnitInfo(Steinberg::Vst::kRootUnitId, info);
    if (res == Steinberg::kResultOk && info.programListId != Steinberg::Vst::kNoProgramListId)
    {
        SUSHI_LOG_INFO("Program list id {}", info.programListId);
        _main_program_list_id = info.programListId;
    }
    /* This is most likely 0, but query and store for good measure as we might want
     * to support multiple program lists in the future */
    Steinberg::Vst::ProgramListInfo list_info;
    res = _instance.unit_info()->getProgramListInfo(Steinberg::Vst::kRootUnitId, list_info);
    if (res == Steinberg::kResultOk)
    {
        _supports_programs = true;
        _program_count = list_info.programCount;
        SUSHI_LOG_INFO("Plugin supports internal programs, program count: {}", _program_count);
        _internal_programs = true;
        return true;
    }
    SUSHI_LOG_INFO("No program list info, returned {}", res);
    return false;
}

bool Vst3xWrapper::_setup_file_program_handling()
{
    _program_files = enumerate_patches(_instance.name(), _instance.vendor());
    if (!_program_files.empty())
    {
        _supports_programs = true;
        _file_based_programs = true;
        _program_count = static_cast<int>(_program_files.size());
        SUSHI_LOG_INFO("Using external file programs, {} program files found", _program_files.size());
        return true;
    }
    return false;
}

void Vst3xWrapper::_forward_events(Steinberg::Vst::ProcessData& data)
{
    int event_count = data.outputEvents->getEventCount();
    for (int i = 0; i < event_count; ++i)
    {
        Steinberg::Vst::Event vst_event;
        if (data.outputEvents->getEvent(i, vst_event) == Steinberg::kResultOk)
        {
            switch (vst_event.type)
            {
                case Steinberg::Vst::Event::EventTypes::kNoteOnEvent:
                    if (maybe_output_gate_event(vst_event.noteOn.channel, vst_event.noteOn.pitch, true) == false)
                    {
                        output_event(RtEvent::make_note_on_event(0, vst_event.sampleOffset,
                                                                    vst_event.noteOn.channel,
                                                                    vst_event.noteOn.pitch,
                                                                    vst_event.noteOn.velocity));
                    }
                    break;

                case Steinberg::Vst::Event::EventTypes::kNoteOffEvent:
                    if (maybe_output_gate_event(vst_event.noteOn.channel, vst_event.noteOn.pitch, false) == false)
                    {
                        output_event(RtEvent::make_note_off_event(0, vst_event.sampleOffset,
                                                                     vst_event.noteOff.channel,
                                                                     vst_event.noteOff.pitch,
                                                                     vst_event.noteOff.velocity));
                    }
                    break;

                case Steinberg::Vst::Event::EventTypes::kPolyPressureEvent:
                    output_event(RtEvent::make_note_aftertouch_event(0, vst_event.sampleOffset,
                                                                     vst_event.polyPressure.channel,
                                                                     vst_event.polyPressure.pitch,
                                                                     vst_event.polyPressure.pressure));
                    break;

                default:
                    break;
            }
        }
    }
}

void Vst3xWrapper::_forward_params(Steinberg::Vst::ProcessData& data)
{
    int param_count = data.outputParameterChanges->getParameterCount();
    for (int i = 0; i < param_count; ++i)
    {
        auto queue = data.outputParameterChanges->getParameterData(i);
        auto id = queue->getParameterId();
        int points = queue->getPointCount();
        if (points > 0)
        {
            double value;
            int offset;
            auto res = queue->getPoint(points - 1, offset, value);
            if (res == Steinberg::kResultOk)
            {
                if (maybe_output_cv_value(id, value) == false)
                {
                    auto float_value = static_cast<float>(value);
                    auto e = RtEvent::make_parameter_change_event(this->id(), 0, id, float_value);
                    output_event(e);
                    _parameter_update_queue.push({id, float_value});
                    _notify_parameter_change = true;
                }
            }
        }
    }
}

void Vst3xWrapper::_fill_processing_context()
{
    auto transport = _host_control.transport();
    auto context = _process_data.processContext;
    *context = {};
    auto ts = transport->time_signature();

    context->state = SUSHI_HOST_TIME_CAPABILITIES | (transport->playing()? Steinberg::Vst::ProcessContext::kPlaying : 0);
    context->sampleRate             = _sample_rate;
    context->projectTimeSamples     = transport->current_samples();
    context->systemTime             = std::chrono::nanoseconds(transport->current_process_time()).count();
    context->continousTimeSamples   = transport->current_samples();
    context->projectTimeMusic       = transport->current_beats();
    context->barPositionMusic       = transport->current_bar_start_beats();
    context->tempo                  = transport->current_tempo();
    context->timeSigNumerator       = ts.numerator;
    context->timeSigDenominator     = ts.denominator;
}

inline void Vst3xWrapper::_add_parameter_change(Steinberg::Vst::ParamID id, float value, int sample_offset)
{
    int index;
    auto param_queue = _in_parameter_changes.addParameterData(id, index);
    if (param_queue)
    {
        param_queue->addPoint(sample_offset, value, index);
    }
}

void Vst3xWrapper::set_parameter_change(ObjectId param_id, float value)
{
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, this->id(), param_id, value, IMMEDIATE_PROCESS);
    _host_control.post_event(event);
}

bool Vst3xWrapper::_sync_processor_to_controller()
{
    Steinberg::MemoryStream stream;
    if (_instance.component()->getState (&stream) == Steinberg::kResultTrue)
    {
        stream.seek(0, Steinberg::MemoryStream::kIBSeekSet, nullptr);
        auto res = _instance.controller()->setComponentState (&stream);
        return res == Steinberg::kResultTrue? true : false;
    }
    SUSHI_LOG_WARNING("Failed to get state from processor");
    return false;
}

void Vst3xWrapper::_program_change_callback(Event* event, int status)
{
    if (status == EventStatus::HANDLED_OK)
    {
        auto typed_event = static_cast<ParameterChangeEvent*>(event);
        _current_program = static_cast<int>(typed_event->float_value() * _program_count);
        SUSHI_LOG_INFO("Set program to {} completed, {}", _current_program, typed_event->parameter_id());
        _instance.controller()->setParamNormalized(_program_change_parameter.id, typed_event->float_value());
        Steinberg::Vst::HostMessage message;
        message.setMessageID("idle");
        if (_instance.notify_processor(&message) == false)
        {
            SUSHI_LOG_ERROR("Idle message returned error");
        }
        return;
    }
    SUSHI_LOG_INFO("Set program failed with status: {}", status);
}

int Vst3xWrapper::_parameter_update_callback(EventId /*id*/)
{
    ParameterUpdate update;
    int res = 0;
    while (_parameter_update_queue.pop(update))
    {
        res |= _instance.controller()->setParamNormalized(update.id, update.value);
    }
    return res == Steinberg::kResultOk? EventStatus::HANDLED_OK : EventStatus::ERROR;
}

void Vst3xWrapper::_set_program_state(int program_id, RtState* rt_state, bool realtime_running)
{
    if (_internal_programs && _program_change_parameter.supported)
    {
        float normalised_id = program_id / static_cast<float>(_program_count);
        _instance.controller()->setParamNormalized(_program_change_parameter.id, normalised_id);
        _current_program = program_id;
        if (realtime_running)
        {
            rt_state->add_parameter_change(_program_change_parameter.id, normalised_id);
        }
        else
        {
            _add_parameter_change(_program_change_parameter.id, normalised_id, 0);
        }
    }
    else // file based programs
    {
        this->set_program(program_id);
    }
}

void Vst3xWrapper::_set_bypass_state(bool bypassed, RtState* rt_state, bool realtime_running)
{
    _bypass_manager.set_bypass(bypassed, _sample_rate);
    if (_bypass_parameter.supported)
    {
        float float_bypass = bypassed ? 1.0f : 0.0f;
        _instance.controller()->setParamNormalized(_bypass_parameter.id, float_bypass);
        if (realtime_running)
        {
            rt_state->add_parameter_change(_bypass_parameter.id, float_bypass);
        }
        else
        {
            _add_parameter_change(_bypass_parameter.id, float_bypass, 0);
        }
    }
}

void Vst3xWrapper::_set_binary_state(std::vector<std::byte>& state)
{
    /* A primer on Vst3 states:
     * Component.setState() sets the state of the audio processing part (param values, etc)
     * Controller.setComponentState() sets the Controllers own mirror of parameter values.
     * Controller.setState() only sets the editors internal state and is not called in sushi
     *
     * State functions are always called from a non-rt thread according to
     * https://developer.steinberg.help/display/VST/Frequently+Asked+Questions */

    auto stream = Steinberg::MemoryStream(reinterpret_cast<void*>(state.data()), state.size());
    [[maybe_unused]] auto res = _instance.controller()->setComponentState(&stream);
    SUSHI_LOG_ERROR_IF(res != Steinberg::kResultOk, "Failed to set component state on controller ({})", res);

    stream.seek(0, Steinberg::MemoryStream::kIBSeekSet, nullptr);
    res = _instance.component()->setState(&stream);
    SUSHI_LOG_ERROR_IF(res , "Failed to set component state ({})", res);
    _host_control.post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_UPDATED,
                                                             this->id(), 0, IMMEDIATE_PROCESS));
}

void Vst3xWrapper::_set_state_rt(Vst3xRtState* state)
{
    if (_state_parameter_changes)
    {
        // If a parameter batch is already queued, just throw it away and use the new one.
        async_delete(_state_parameter_changes);
    }
    _state_parameter_changes = state;
    notify_state_change_rt();
}

Steinberg::Vst::SpeakerArrangement speaker_arr_from_channels(int channels)
{
    switch (channels)
    {
        case 0:
            return Steinberg::Vst::SpeakerArr::kEmpty;
        case 1:
            return Steinberg::Vst::SpeakerArr::kMono;
        case 2:
            return Steinberg::Vst::SpeakerArr::kStereo;
        case 3:
            return Steinberg::Vst::SpeakerArr::k30Music;
        case 4:
            return Steinberg::Vst::SpeakerArr::k40Music;
        case 5:
            return Steinberg::Vst::SpeakerArr::k50;
        case 6:
            return Steinberg::Vst::SpeakerArr::k60Music;
        case 7:
            return Steinberg::Vst::SpeakerArr::k70Music;
        default:
            return Steinberg::Vst::SpeakerArr::k80Music;
    }
}

} // end namespace vst3
} // end namespace sushi
