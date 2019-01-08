#include "engine/controller.h"
#include "engine/base_engine.h"

#include "logging.h"


MIND_GET_LOGGER_WITH_MODULE_NAME("controller")

namespace sushi {


/* Convenience conversion functions between external and internal
 * enums and data structs */
inline ext::ParameterType to_external(const sushi::ParameterType type)
{
    switch (type)
    {
        case ParameterType::FLOAT:      return ext::ParameterType::FLOAT;
        case ParameterType::INT:        return ext::ParameterType::INT;
        case ParameterType::BOOL:       return ext::ParameterType::BOOL;
        case ParameterType::SELECTION:  return ext::ParameterType::INT;
        case ParameterType::STRING:     return ext::ParameterType::STRING_PROPERTY;
        case ParameterType::DATA:       return ext::ParameterType::DATA_PROPERTY;
        default:                        return ext::ParameterType::FLOAT;
    }
}

inline ext::PlayingMode to_external(const sushi::PlayingMode mode)
{
    switch (mode)
    {
        case PlayingMode::STOPPED:      return ext::PlayingMode::STOPPED;
        case PlayingMode::PLAYING:      return ext::PlayingMode::PLAYING;
        case PlayingMode::RECORDING:    return ext::PlayingMode::RECORDING;
        default:                        return ext::PlayingMode::PLAYING;
    }
}

inline sushi::PlayingMode to_internal(const ext::PlayingMode mode)
{
    switch (mode)
    {
        case ext::PlayingMode::STOPPED:   return sushi::PlayingMode::STOPPED;
        case ext::PlayingMode::PLAYING:   return sushi::PlayingMode::PLAYING;
        case ext::PlayingMode::RECORDING: return sushi::PlayingMode::RECORDING;
        default:                          return sushi::PlayingMode::PLAYING;
    }
}

inline ext::SyncMode to_external(const sushi::SyncMode mode)
{
    switch (mode)
    {
        case SyncMode::INTERNAL:     return ext::SyncMode::INTERNAL;
        case SyncMode::MIDI_SLAVE:   return ext::SyncMode::MIDI;
        case SyncMode::ABLETON_LINK: return ext::SyncMode::LINK;
        default:                     return ext::SyncMode::INTERNAL;
    }
}

inline sushi::SyncMode to_internal(const ext::SyncMode mode)
{
    switch (mode)
    {
        case ext::SyncMode::INTERNAL: return sushi::SyncMode::INTERNAL;
        case ext::SyncMode::MIDI:     return sushi::SyncMode::MIDI_SLAVE;
        case ext::SyncMode::LINK:     return sushi::SyncMode::ABLETON_LINK;
        default:                      return sushi::SyncMode::INTERNAL;
    }
}

inline ext::TimeSignature to_external(sushi::TimeSignature internal)
{
    return {internal.numerator, internal.denominator};
}

inline sushi::TimeSignature to_internal(ext::TimeSignature ext)
{
    return {ext.numerator, ext.denominator};
}

/*inline ext::CpuTimings to_external(sushi::performance::ProcessTimings& internal)
{
    return {internal.avg_case, internal.min_case, internal.max_case};
}

inline sushi::performance::ProcessTimings to_internal(ext::CpuTimings& ext)
{
    return {ext.avg, ext.min, ext.min};
}*/



Controller::Controller(engine::BaseEngine* engine) : _engine{engine}
{
    _event_dispatcher = _engine->event_dispatcher();
    _transport = _engine->transport();
}

Controller::~Controller() = default;

float Controller::get_samplerate() const
{
    MIND_LOG_DEBUG("get_samplerate called");
    return _engine->sample_rate();
}

ext::PlayingMode Controller::get_playing_mode() const
{
    MIND_LOG_DEBUG("get_playing_mode called");
    return to_external(_transport->playing_mode());
}

void Controller::set_playing_mode(ext::PlayingMode playing_mode)
{
    MIND_LOG_DEBUG("set_playing_mode called");
    auto event = new SetEnginePlayingModeStateEvent(to_internal(playing_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

ext::SyncMode Controller::get_sync_mode() const
{
    MIND_LOG_DEBUG("get_sync_mode called");
    return to_external(_transport->sync_mode());
}

void Controller::set_sync_mode(ext::SyncMode sync_mode)
{
    auto event = new SetEngineSyncModeEvent(to_internal(sync_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

float Controller::get_tempo() const
{
    MIND_LOG_DEBUG("get_tempo called");
    return _transport->current_tempo();
}

ext::ControlStatus Controller::set_tempo(float tempo)
{
    MIND_LOG_DEBUG("set_tempo called with tempo {}", tempo);
    auto event = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::TimeSignature Controller::get_time_signature() const
{
    MIND_LOG_DEBUG("get_time_signature called");
    return to_external(_transport->current_time_signature());
}

ext::ControlStatus Controller::set_time_signature(ext::TimeSignature signature)
{
    MIND_LOG_DEBUG("set_time_signature called with signature {}/{}", signature.numerator, signature.denominator);
    auto event = new SetEngineTimeSignatureEvent(to_internal(signature), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

bool Controller::get_timing_statistics_enabled()
{
    MIND_LOG_DEBUG("get_timing_statistics_enabled called");
    return true;
}

void Controller::set_timing_statistics_enabled(bool enabled) const
{
    MIND_LOG_DEBUG("set_timing_statistics_enabled called with {}", enabled);
    _engine->enable_timing_statistics(enabled);
}

std::vector<ext::TrackInfo> Controller::get_tracks() const
{
    MIND_LOG_DEBUG("get_tracks called");
    auto& tracks = _engine->all_tracks();
    std::vector<ext::TrackInfo> returns;
    for (const auto& t : tracks)
    {
        ext::TrackInfo info;
        info.id = t->id();
        info.name = t->name();
        info.label = t->label();
        info.input_busses = t->input_busses();
        info.input_channels = t->input_channels();
        info.output_busses = t->output_channels();
        info.output_channels = t->output_channels();
        info.processor_count = t->process_chain().size();
        returns.push_back(info);
    }
    return returns;
}

ext::ControlStatus Controller::send_note_on(int track_id, int note, float velocity)
{
    MIND_LOG_DEBUG("send_note_on called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, static_cast<ObjectId>(track_id),
                                   note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_note_off(int track_id, int note, float velocity)
{
    MIND_LOG_DEBUG("send_note_off called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, static_cast<ObjectId>(track_id),
                                   note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;}

ext::ControlStatus Controller::send_note_aftertouch(int track_id, int note, float value)
{
    MIND_LOG_DEBUG("send_note_aftertouch called with track {}, note {}, value {}", track_id, note, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   note, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;}

ext::ControlStatus Controller::send_aftertouch(int track_id, float value)
{
    MIND_LOG_DEBUG("send_aftertouch called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_pitch_bend(int track_id, float value)
{
    MIND_LOG_DEBUG("send_pitch_bend called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, static_cast<ObjectId>(track_id),
                                   value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_modulation(int track_id, float value)
{
    MIND_LOG_DEBUG("send_modulation called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, static_cast<ObjectId>(track_id),
                                   value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::CpuTimings Controller::get_engine_timings() const
{
    MIND_LOG_DEBUG("get_engine_timings called, returning ");
    return {0.1, 0.2, 05};
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_track_timings(int /*track_id*/) const
{
    MIND_LOG_DEBUG("get_track_timings called, returning ");
    return {ext::ControlStatus::OK, ext::CpuTimings{0.05, 0.1, 0.02}};
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_processor_timings(int /*processor_id*/) const
{
    MIND_LOG_DEBUG("get_processor_timings called, returning ");
    return {ext::ControlStatus::OK, ext::CpuTimings{0.05, 0.1, 0.02}};
}

ext::ControlStatus Controller::reset_all_timings()
{
    MIND_LOG_DEBUG("reset_all_timings called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::reset_track_timings(int /*track_id*/)
{
    MIND_LOG_DEBUG("reset_track_timings called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::reset_processor_timings(int /*processor_id*/)
{
    MIND_LOG_DEBUG("reset_processor_timings called, returning ");
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, int> Controller::get_track_id(const std::string& track_name) const
{
    MIND_LOG_DEBUG("get_track_id called with track {}", track_name);
    return this->get_processor_id(track_name);
}

std::pair<ext::ControlStatus, ext::TrackInfo> Controller::get_track_info(int track_id) const
{
    MIND_LOG_DEBUG("get_track_info called with track {}", track_id);
    ext::TrackInfo info;
    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (static_cast<int>(track->id()) == track_id)
        {
            info.label = track->label();
            info.name = track->name();
            info.id = track->id();
            info.processor_count = track->process_chain().size();
            info.input_channels = track->input_channels();
            info.input_busses = track->input_busses();
            info.output_channels = track->output_channels();
            info.output_busses = track->output_busses();
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> Controller::get_track_processors(int track_id) const
{
    MIND_LOG_DEBUG("get_track_processors called for track: {}", track_id);
    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (static_cast<int>(track->id()) == track_id)
        {
            std::vector<ext::ProcessorInfo> infos;
            const auto& procs = track->process_chain();
            for (const auto& processor : procs)
            {
                ext::ProcessorInfo info;
                info.label = processor->label();
                info.name = processor->name();
                info.id = processor->id();
                info.parameter_count = processor->parameter_count();
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ProcessorInfo>()};
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> Controller::get_track_parameters(int track_id) const
{
    MIND_LOG_DEBUG("get_track_parameters called for track: {}", track_id);
    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (static_cast<int>(track->id()) == track_id)
        {
            std::vector<ext::ParameterInfo> infos;
            const auto& params = track->all_parameters();
            for (const auto& param : params)
            {
                ext::ParameterInfo info;
                info.label = param->label();
                info.name = param->name();
                info.id = param->id();
                info.type = ext::ParameterType::FLOAT;
                info.min_range = param->min_range();
                info.max_range = param->max_range();
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> Controller::get_processor_id(const std::string& processor_name) const
{
    MIND_LOG_DEBUG("get_processor_id called with processor {}", processor_name);
    auto [status, id] = _engine->processor_id_from_name(processor_name);
    if (status == engine::EngineReturnStatus::OK)
    {
        return {ext::ControlStatus::OK, id};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::ProcessorInfo> Controller::get_processor_info(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_info called with processor {}", processor_id);
    ext::ProcessorInfo info;
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, info};
    }
    info.id = processor_id;
    info.name = processor->name();
    info.label = processor->label();
    info.parameter_count = processor->parameter_count();
    info.program_count = processor->supports_programs()? processor->program_count() : 0;
    return {ext::ControlStatus::OK, info};
}

std::pair<ext::ControlStatus, bool> Controller::get_processor_bypass_state(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_bypass_state called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, false};
    }
    return {ext::ControlStatus::OK, processor->bypassed()};
}

ext::ControlStatus Controller::set_processor_bypass_state(int /*processor_id*/, bool /*bypass_enabled*/)
{
    MIND_LOG_DEBUG("set_processor_bypass_state called, returning ");
    return {ext::ControlStatus::OK};
}

std::pair<ext::ControlStatus, int> Controller::get_processor_current_program(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_current_program called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, 0};
    }
    if (processor->supports_programs())
    {
        return {ext::ControlStatus::OK, processor->current_program()};
    }
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, 0};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_current_program_name(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_current_program_name called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, ""};
    }
    if (processor->supports_programs())
    {
        return {ext::ControlStatus::OK, processor->current_program_name()};
    }
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, ""};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_program_name(int processor_id, int program_id) const
{
    MIND_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, ""};
    }
    else if (processor->supports_programs() == false)
    {
        return {ext::ControlStatus::UNSUPPORTED_OPERATION, ""};
    }
    auto [status, name] = processor->program_name(program_id);
    if (status == ProcessorReturnCode::OK)
    {
        return {ext::ControlStatus::OK, std::move(name)};
    }
    return {ext::ControlStatus::OUT_OF_RANGE, ""};
}

std::pair<ext::ControlStatus, std::vector<std::string>> Controller::get_processor_programs(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, std::vector<std::string>()};
    }
    else if (processor->supports_programs() == false)
    {
        return {ext::ControlStatus::UNSUPPORTED_OPERATION, std::vector<std::string>()};
    }
    auto [status, names] = processor->all_program_names();
    if (status == ProcessorReturnCode::OK)
    {
        return {ext::ControlStatus::OK, std::move(names)};
    }
    return {ext::ControlStatus::OUT_OF_RANGE, std::vector<std::string>()};
}

ext::ControlStatus Controller::set_processor_program(int /*processor_id*/, int /*program_id*/)
{
    MIND_LOG_DEBUG("set_processor_program called, returning ");
    // TODO - send an event here if program change is to happen in the rt thread
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>>
Controller::get_processor_parameters(int processor_id) const
{
    MIND_LOG_DEBUG("get_processor_parameters called with processor {}", processor_id);
    const auto& procs = _engine->all_processors();
    for (const auto& proc : procs)
    {
        if (static_cast<int>(proc.second->id()) == processor_id)
        {
            std::vector<ext::ParameterInfo> infos;
            const auto& params = proc.second->all_parameters();
            for (const auto& param : params)
            {
                ext::ParameterInfo info;
                info.label = param->label();
                info.name = param->name();
                info.id = param->id();
                info.type = ext::ParameterType::FLOAT;
                info.min_range = param->min_range();
                info.max_range = param->max_range();
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> Controller::get_parameter_id(int processor_id, const std::string& parameter_name) const
{
    MIND_LOG_DEBUG("get_parameter_id called with processor {} and parameter {}", processor_id, parameter_name);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, 0};
    }
    auto descr = processor->parameter_from_name(parameter_name);
    if (descr != nullptr)
    {
        return {ext::ControlStatus::OK, descr->id()};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::ParameterType> Controller::get_parameter_type(int processor_id, int parameter_id) const
{
    MIND_LOG_DEBUG("get_parameter_type called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, ext::ParameterType::FLOAT};
    }
    auto descr = processor->parameter_from_id(static_cast<ObjectId>(parameter_id));
    if (descr != nullptr)
    {
        return {ext::ControlStatus::OK, to_external(descr->type())};
    }
    return {ext::ControlStatus::NOT_FOUND, ext::ParameterType::FLOAT};
}

std::pair<ext::ControlStatus, ext::ParameterInfo> Controller::get_parameter_info(int processor_id, int parameter_id) const
{
    MIND_LOG_DEBUG("get_parameter_info called with processor {} and parameter {}", processor_id, parameter_id);
    ext::ParameterInfo info;
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto descr = processor->parameter_from_id(static_cast<ObjectId>(parameter_id));
        if (descr != nullptr)
        {
            info.id = descr->id();
            info.label = descr->label();
            info.name = descr->name();
            info.unit = ""; // TODO - implement
            info.type = to_external(descr->type());
            info.min_range = descr->min_range();
            info.max_range = descr->max_range();
            info.automatable =  descr->type() == ParameterType::FLOAT || // TODO - this might not be the way we eventually want it
                                descr->type() == ParameterType::INT   ||
                                descr->type() == ParameterType::BOOL  ||
                                descr->type() == ParameterType::SELECTION;
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, float> Controller::get_parameter_value(int processor_id, int parameter_id) const
{
    MIND_LOG_DEBUG("get_parameter_value called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, float> Controller::get_parameter_value_normalised(int processor_id, int parameter_id) const
{
    MIND_LOG_DEBUG("get_parameter_value_normalised called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_normalised(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, std::string> Controller::get_parameter_value_as_string(int processor_id, int parameter_id) const
{
    MIND_LOG_DEBUG("get_parameter_value_as_string called with processor {} and parameter {}", processor_id, parameter_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor != nullptr)
    {
        auto[status, value] = processor->parameter_value_formatted(static_cast<ObjectId>(parameter_id));
        if (status == ProcessorReturnCode::OK)
        {
            return {ext::ControlStatus::OK, value};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, std::string> Controller::get_string_property_value(int /*processor_id*/, int /*parameter_id*/) const
{
    MIND_LOG_DEBUG("get_string_property_value called, returning ");
    return {ext::ControlStatus::OK, "helloworld"};
}

ext::ControlStatus Controller::set_parameter_value(int processor_id, int parameter_id, float value)
{
    MIND_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_parameter_value_normalised(int processor_id, int parameter_id, float value)
{
    // TODO -  this is exactly the same as set_parameter_value
    MIND_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id, value);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_string_property_value(int /*processor_id*/, int /*parameter_id*/, std::string /*value*/)
{
    MIND_LOG_DEBUG("set_string_property_value called, returning ");
    return ext::ControlStatus::OK;
}

}// namespace sushi