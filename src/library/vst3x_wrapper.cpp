#ifdef SUSHI_BUILD_WITH_VST3

#include <fstream>
#include <string>
#include <climits>

#include <dirent.h>
#include <cstdlib>

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

MIND_GET_LOGGER_WITH_MODULE_NAME("vst3");

// Convert a Steinberg 128 char unicode string to 8 bit ascii std::string
std::string to_ascii_str(Steinberg::Vst::String128 wchar_buffer)
{
    char char_buf[128] = {};
    Steinberg::UString128 str(wchar_buffer, 128);
    str.toAscii(char_buf, VST_NAME_BUFFER_SIZE);
    return std::string(char_buf);
}

// Get all vst3 preset locations in the right priority order
// See Steinberg documentation of "Preset locations".
std::vector<std::string> get_preset_locations()
{
    std::vector<std::string> locations;
    char* home_dir = getenv("HOME");
    if (home_dir != nullptr)
    {
        locations.emplace_back(std::string(home_dir) + "/.vst3/presets/");
    }
    MIND_LOG_WARNING_IF(home_dir == nullptr, "Failed to get home directory");
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
    MIND_LOG_WARNING_IF(path_length <= 0, "Failed to get binary directory");
    return locations;
}

std::string extract_preset_name(const std::string& path)
{
    auto fname_pos = path.find_last_of("/") + 1;
    return path.substr(fname_pos, path.length() - fname_pos - VST_PRESET_SUFFIX_LENGTH);
}

// Recursively search subdirs for preset files
void add_patches(const std::string& path, std::vector<std::string>& patches)
{
    MIND_LOG_INFO("Looking for presets in: {}", path);
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
                MIND_LOG_DEBUG("Reading vst preset patch: {}", patch_name);
                patches.emplace_back(std::move(path + "/" + patch_name));
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
}

ProcessorReturnCode Vst3xWrapper::init(float sample_rate)
{
    _sample_rate = sample_rate;
    bool loaded = _instance.load_plugin(_plugin_load_path, _plugin_load_name);
    if (!loaded)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_LOAD_ERROR;
    }
    set_name(_instance.name());
    set_label(_instance.name());

    if (!_setup_audio_busses() || !_setup_event_busses())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    auto res = _instance.component()->setActive(Steinberg::TBool(true));
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to activate component with error code: {}", res);
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    res = _instance.controller()->setComponentHandler(&_component_handler);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to set component handler with error code: {}", res);
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    if (!_sync_processor_to_controller())
    {
        MIND_LOG_WARNING("failed to sync controller");
    }

    if (!_setup_channels())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
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
        MIND_LOG_ERROR("Error setting sample rate to {}", sample_rate);
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
        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            // TODO - Invoke midi decoder here, vst3 doesn't support raw midi
            // Or do nothing, no reason to send raw midi to at VST3 plugin
        }
        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _bypass_manager.set_bypass(bypassed, _sample_rate);
            break;
        }
        default:
            break;
    }
}

void Vst3xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_process_data.inputParameterChanges->getParameterCount() > 0)
    {
        auto e = RtEvent::make_async_work_event(&Vst3xWrapper::parameter_update_callback, this->id(), this);
        output_event(e);
    }
    if(_bypass_parameter.supported == false && _bypass_manager.should_process() == false)
    {
        bypass_process(in_buffer, out_buffer);
    }
    else
    {
        _fill_processing_context();
        _process_data.assign_buffers(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        _instance.processor()->process(_process_data);
        if(_bypass_parameter.supported == false && _bypass_manager.should_ramp())
        {
            _bypass_manager.crossfade_output(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        }
        _forward_events(_process_data);
        _forward_params(_process_data);
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
    auto res = _instance.processor()->setProcessing(Steinberg::TBool(enabled));
    if (res == Steinberg::kResultOk)
    {
        _enabled = enabled;
    }
}

void Vst3xWrapper::set_bypassed(bool bypassed)
{
    assert(twine::is_current_thread_realtime() == false);
    if(_bypass_parameter.supported)
    {
        _host_control.post_event(new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                                          this->id(), _bypass_parameter.id, bypassed? 1.0f : 0, IMMEDIATE_PROCESS));
        _bypass_manager.set_bypass(bypassed, _sample_rate);
    } else
    {
        _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
    }
}

bool Vst3xWrapper::bypassed() const
{
    if (_bypass_parameter.supported)
    {
        float value;
        std::tie(std::ignore, value) = this->parameter_value_normalised(_bypass_parameter.id);
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
    auto value = controller->normalizedParamToPlain(parameter_id, controller->getParamNormalized(parameter_id));
    return {ProcessorReturnCode::OK, static_cast<float>(value)};
}

std::pair<ProcessorReturnCode, float> Vst3xWrapper::parameter_value_normalised(ObjectId parameter_id) const
{
    /* Always returns OK as the default vst3 implementation just returns 0 for invalid parameter ids */
    auto controller = const_cast<PluginInstance*>(&_instance)->controller();
    auto value = controller->getParamNormalized(parameter_id);
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
        MIND_LOG_INFO("Program name {}", program);
        auto mutable_unit = const_cast<PluginInstance*>(&_instance)->unit_info();
        Steinberg::Vst::String128 buffer;
        auto res = mutable_unit->getProgramName(_main_program_list_id, program, buffer);
        if (res == Steinberg::kResultOk)
        {
            MIND_LOG_INFO("Program name returned error {}", res);
            return {ProcessorReturnCode::OK, to_ascii_str(buffer)};
        }
    }
    else if (_supports_programs && _file_based_programs && program < static_cast<int>(_program_files.size()))
    {
        return {ProcessorReturnCode::OK, extract_preset_name(_program_files[program])};
    }
    MIND_LOG_INFO("Set program name failed");
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, ""};
}

std::pair<ProcessorReturnCode, std::vector<std::string>> Vst3xWrapper::all_program_names() const
{
    if (_supports_programs)
    {
        MIND_LOG_INFO("all Program names");
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
                    programs.emplace_back(to_ascii_str(buffer));
                } else
                {
                    MIND_LOG_INFO("Program name returned error {} on {}", res, i);
                    break;
                }
            }
            else if (_file_based_programs)
            {
                programs.emplace_back(extract_preset_name(_program_files[i]));
            }
        }
        MIND_LOG_INFO("Return list with {} programs", programs.size());
        return {ProcessorReturnCode::OK, programs};
    }
    MIND_LOG_INFO("All program names failed");
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
        MIND_LOG_INFO("Set program {}, {}, {}", program, normalised_program_id, _program_change_parameter.id);
        //_instance.controller()->setParamNormalized(_program_change_parameter.id, normalised_program_id);
        return ProcessorReturnCode::OK;
    }
    else if (_file_based_programs && program < static_cast<int>(_program_files.size()))
    {
        MIND_LOG_INFO("Loading file based preset");
        Steinberg::OPtr<Steinberg::IBStream> stream(Steinberg::Vst::FileStream::open(_program_files[program].c_str(), "rb"));
        if (stream == nullptr)
        {
            MIND_LOG_INFO("Failed to load file {}", _program_files[program]);
            return ProcessorReturnCode::ERROR;
        }
        Steinberg::Vst::PresetFile preset_file(stream);
        preset_file.readChunkList();

        bool res = preset_file.restoreControllerState(_instance.controller());
        res &= preset_file.restoreComponentState(_instance.component());
        // Notify the processor of the update with an idle message. This was specific
        // to Retrologue and not part of the Vst3 standard so we might remove it eventually
        Steinberg::Vst::HostMessage message;
        message.setMessageID("idle");
        if (_instance.notify_processor(&message) == false)
        {
            MIND_LOG_ERROR("Idle message returned error");
        }
        if (res)
        {
            _current_program = program;
            return ProcessorReturnCode::OK;
        } else
        {
            MIND_LOG_INFO("restore state returned error");
        }
    }
    MIND_LOG_INFO("Error in program change");
    return ProcessorReturnCode::ERROR;
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
             * also has a 32 bit integer id which is arbitrarily assigned.
             *
             * When doing real time parameter updates, the parameters must be accessed using this
             * id and not its index. Hence the id in the registered ParameterDescriptors
             * store this id and not the index in the processor array like it does for the Vst2
             * wrapper and internal plugins. Hopefully that doesn't cause any issues. */
            auto title = to_ascii_str(info.title);
            if(info.flags & Steinberg::Vst::ParameterInfo::kIsBypass)
            {
                _bypass_parameter.id = info.id;
                _bypass_parameter.supported = true;
                MIND_LOG_INFO("Plugin supports soft bypass");
            }
            else if(info.flags & Steinberg::Vst::ParameterInfo::kIsProgramChange &&
                    _program_change_parameter.supported == false)
            {
                /* For now we only support 1 program change parameter and we're counting on the
                 * first one to be the "major" one. Multitimbral instruments can have multiple
                 * program change parameters, but we'll have to look into how to support that. */
                _program_change_parameter.id = info.id;
                _program_change_parameter.supported = true;
                MIND_LOG_INFO("We have a program change parameter at {}", info.id);
            }
            else if (register_parameter(new FloatParameterDescriptor(title, title, 0, 1, nullptr), info.id))
            {
                MIND_LOG_INFO("Registered parameter {}, id {}", title, info.id);
            }
            else
            {
                MIND_LOG_INFO("Error registering parameter {}.", title);
            }
        }
    }
    // Create a "backwards map" from Vst3 parameter ids to parameter indices
    for (auto param : this->all_parameters())
    {
        _parameters_by_vst3_id[param->id()] = param;
    }
    /* Steinberg decided not support standard midi, nor provide special events for common
     * controller (Pitch bend, mod wheel, etc) instead these are exposed as regular
     * parameters and we can query the plugin for what 'default' midi cc:s these parameters
     * would be mapped to if the plugin was able to handle native midi.
     * So we query the plugin for this and if that's the case, store the id:s of these
     * 'special' parameters so we can map PB and Mod events to them.
     * Currently we dont hide these parameters, unlike the bypass parameter, so they can
     * still be controlled via OSC or other controllers. */
    if (_instance.midi_mapper())
    {
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kCtrlModWheel,
                                                     _mod_wheel_parameter.id) == Steinberg::kResultOk)
        {
            MIND_LOG_INFO("Plugin supports mod wheel parameter mapping");
            _mod_wheel_parameter.supported = true;
        }
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kPitchBend,
                                                     _pitch_bend_parameter.id) == Steinberg::kResultOk)
        {
            MIND_LOG_INFO("Plugin supports pitch bend parameter mapping");
            _pitch_bend_parameter.supported = true;
        }
        if (_instance.midi_mapper()->getMidiControllerAssignment(0, 0, Steinberg::Vst::kAfterTouch,
                                                     _aftertouch_parameter.id) == Steinberg::kResultOk)
        {
            MIND_LOG_INFO("Plugin supports aftertouch parameter mapping");
            _aftertouch_parameter.supported = true;
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_audio_busses()
{
    int input_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput);
    int output_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput);
    MIND_LOG_INFO("Plugin has {} audio input buffers and {} audio output buffers", input_audio_busses, output_audio_busses);
    if (output_audio_busses == 0)
    {
        return false;
    }
    _max_input_channels = 0;
    _max_output_channels = 0;
    /* Setup 1 main output bus and 1 main input bus (if available) */
    Steinberg::Vst::BusInfo info;
    for (int i = 0; i < input_audio_busses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_input_channels = info.channelCount;
            _current_input_channels = _max_input_channels;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin input bus {}", i);
                return false;
            }
            break;
        }
    }
    for (int i = 0; i < output_audio_busses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_output_channels = info.channelCount;
            _current_output_channels = _max_output_channels;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin output bus {}", i);
                return false;
            }
            break;
        }
    }
    MIND_LOG_INFO("Vst3 wrapper ({}) has {} inputs and {} outputs", this->name(), _max_input_channels, _max_output_channels);
    return true;
}

bool Vst3xWrapper::_setup_event_busses()
{
    int input_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kInput);
    int output_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kOutput);
    MIND_LOG_INFO("Plugin has {} event input buffers and {} event output buffers", input_busses, output_busses);
    /* Try to activate all busses here */
    for (int i = 0; i < input_busses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            MIND_LOG_ERROR("Failed to activate plugin input event bus {}", i);
            return false;
        }
    }
    for (int i = 0; i < output_busses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                      Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            MIND_LOG_ERROR("Failed to activate plugin output event bus {}", i);
            return false;
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_channels()
{
    MIND_LOG_INFO("Vst3 wrapper ({}) setting up {} inputs and {} outputs", this->name(), _current_input_channels, _current_output_channels);
    Steinberg::Vst::SpeakerArrangement input_arr = speaker_arr_from_channels(_current_input_channels);
    Steinberg::Vst::SpeakerArrangement output_arr = speaker_arr_from_channels(_current_output_channels);

    /* numIns and numOuts refer to the number of busses, not channels, the docs are very vague on this point */
    auto res = _instance.processor()->setBusArrangements(&input_arr, (_max_input_channels == 0)? 0:1, &output_arr, 1);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to set a valid channel arrangement");
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
        MIND_LOG_ERROR("Error setting up processing, error code: {}", res);
        return false;
    }
    return true;
}

bool Vst3xWrapper::_setup_internal_program_handling()
{
    if (_instance.unit_info() == nullptr || _program_change_parameter.supported == false)
    {
        MIND_LOG_INFO("No unit info or program change parameter");
        return false;
    }
    if (_instance.unit_info()->getProgramListCount() == 0)
    {
        MIND_LOG_INFO("ProgramListCount is 0");
        return false;
    }
    _main_program_list_id = 0;
    Steinberg::Vst::UnitInfo info;
    auto res = _instance.unit_info()->getUnitInfo(Steinberg::Vst::kRootUnitId, info);
    if (res == Steinberg::kResultOk && info.programListId != Steinberg::Vst::kNoProgramListId)
    {
        MIND_LOG_INFO("Program list id {}", info.programListId);
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
        MIND_LOG_INFO("Plugin supports internal programs, program count: {}", _program_count);
        _internal_programs = true;
        return true;
    }
    MIND_LOG_INFO("No program list info, returned {}", res);
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
        MIND_LOG_INFO("Using external file programs, {} program files found", _program_files.size());
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
                    auto e = RtEvent::make_parameter_change_event(this->id(), 0, id, static_cast<float>(value));
                    output_event(e);
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
    auto ts = transport->current_time_signature();

    context->state = SUSHI_HOST_TIME_CAPABILITIES | transport->playing()? Steinberg::Vst::ProcessContext::kPlaying : 0;
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

bool Vst3xWrapper::_sync_controller_to_processor()
{
    Steinberg::MemoryStream stream;
    if (_instance.controller()->getState (&stream) == Steinberg::kResultTrue)
    {
        stream.seek(0, Steinberg::MemoryStream::kIBSeekCur, nullptr);
        auto res = _instance.component()->setState (&stream);
        return res == Steinberg::kResultTrue? true : false;
    }
    MIND_LOG_WARNING("Failed to get state from controller");
    return false;
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
    MIND_LOG_WARNING("Failed to get state from processor");
    return false;
}

void Vst3xWrapper::_program_change_callback(Event* event, int status)
{
    if (status == EventStatus::HANDLED_OK)
    {
        auto typed_event = static_cast<ParameterChangeEvent*>(event);
        _current_program = static_cast<int>(typed_event->float_value() * _program_count);
        MIND_LOG_INFO("Set program to {} completed, {}", _current_program, typed_event->parameter_id());
        _instance.controller()->setParamNormalized(_program_change_parameter.id, typed_event->float_value());
        Steinberg::Vst::HostMessage message;
        message.setMessageID("idle");
        if (_instance.notify_processor(&message) == false)
        {
            MIND_LOG_ERROR("Idle message returned error");
        }
        return;
    }
    MIND_LOG_INFO("Set program failed with status: {}", status);
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

#endif //SUSHI_BUILD_WITH_VST3
#ifndef SUSHI_BUILD_WITH_VST3
#include "library/vst3x_wrapper.h"
#include "logging.h"
namespace sushi {
namespace vst3 {
MIND_GET_LOGGER;
ProcessorReturnCode Vst3xWrapper::init(float /*sample_rate*/)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Vst 3 support!");
    return ProcessorReturnCode::UNSUPPORTED_OPERATION;
}}}
#endif
