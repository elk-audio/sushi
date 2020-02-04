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
 * @Brief Controller object for external control of sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "engine/controller.h"
#include "engine/base_engine.h"

#include "logging.h"


SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller")

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
        case SyncMode::GATE_INPUT:   return ext::SyncMode::GATE;
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
        case ext::SyncMode::GATE:     return sushi::SyncMode::GATE_INPUT;
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

inline ext::CpuTimings to_external(sushi::performance::ProcessTimings& internal)
{
    return {internal.avg_case, internal.min_case, internal.max_case};
}

Controller::Controller(engine::BaseEngine* engine) : _engine{engine}
{
    _event_dispatcher = _engine->event_dispatcher();
    _transport = _engine->transport();
    _performance_timer = engine->performance_timer();
}

Controller::~Controller() = default;

float Controller::get_samplerate() const
{
    SUSHI_LOG_DEBUG("get_samplerate called");
    return _engine->sample_rate();
}

ext::PlayingMode Controller::get_playing_mode() const
{
    SUSHI_LOG_DEBUG("get_playing_mode called");
    return to_external(_transport->playing_mode());
}

void Controller::set_playing_mode(ext::PlayingMode playing_mode)
{
    SUSHI_LOG_DEBUG("set_playing_mode called");
    auto event = new SetEnginePlayingModeStateEvent(to_internal(playing_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

ext::SyncMode Controller::get_sync_mode() const
{
    SUSHI_LOG_DEBUG("get_sync_mode called");
    return to_external(_transport->sync_mode());
}

void Controller::set_sync_mode(ext::SyncMode sync_mode)
{
    SUSHI_LOG_DEBUG("set_sync_mode called");
    auto event = new SetEngineSyncModeEvent(to_internal(sync_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

float Controller::get_tempo() const
{
    SUSHI_LOG_DEBUG("get_tempo called");
    return _transport->current_tempo();
}

ext::ControlStatus Controller::set_tempo(float tempo)
{
    SUSHI_LOG_DEBUG("set_tempo called with tempo {}", tempo);
    auto event = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::TimeSignature Controller::get_time_signature() const
{
    SUSHI_LOG_DEBUG("get_time_signature called");
    return to_external(_transport->current_time_signature());
}

ext::ControlStatus Controller::set_time_signature(ext::TimeSignature signature)
{
    SUSHI_LOG_DEBUG("set_time_signature called with signature {}/{}", signature.numerator, signature.denominator);
    auto event = new SetEngineTimeSignatureEvent(to_internal(signature), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

bool Controller::get_timing_statistics_enabled()
{
    SUSHI_LOG_DEBUG("get_timing_statistics_enabled called");
    return _performance_timer->enabled();
}

void Controller::set_timing_statistics_enabled(bool enabled) const
{
    SUSHI_LOG_DEBUG("set_timing_statistics_enabled called with {}", enabled);
    // TODO - do this by events instead.
    _engine->performance_timer()->enable(enabled);
}

std::vector<ext::TrackInfo> Controller::get_tracks() const
{
    SUSHI_LOG_DEBUG("get_tracks called");
    auto tracks = _engine->all_tracks();
    std::vector<ext::TrackInfo> returns;
    for (const auto& t : tracks)
    {
        ext::TrackInfo info;
        info.id = t->id();
        info.name = t->name();
        info.label = t->label();
        info.input_busses = t->input_busses();
        info.input_channels = t->input_channels();
        info.output_busses = t->output_busses();
        info.output_channels = t->output_channels();
        info.processor_count = 0; //static_cast<int>(t->process_chain().size()); // TODO - fix this eventually
        returns.push_back(info);
    }
    return returns;
}

ext::ControlStatus Controller::send_note_on(int track_id, int channel, int note, float velocity)
{
    SUSHI_LOG_DEBUG("send_note_on called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, static_cast<ObjectId>(track_id),
                                   channel, note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_note_off(int track_id, int channel, int note, float velocity)
{
    SUSHI_LOG_DEBUG("send_note_off called with track {}, note {}, velocity {}", track_id, note, velocity);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, static_cast<ObjectId>(track_id),
                                   channel, note, velocity, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;}

ext::ControlStatus Controller::send_note_aftertouch(int track_id, int channel, int note, float value)
{
    SUSHI_LOG_DEBUG("send_note_aftertouch called with track {}, note {}, value {}", track_id, note, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   channel, note, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_aftertouch(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_aftertouch called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_pitch_bend(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_pitch_bend called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_modulation(int track_id, int channel, float value)
{
    SUSHI_LOG_DEBUG("send_modulation called with track {} and value {}", track_id, value);
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, static_cast<ObjectId>(track_id),
                                   channel, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_engine_timings() const
{
    SUSHI_LOG_DEBUG("get_engine_timings called, returning ");
    return _get_timings(engine::ENGINE_TIMING_ID);
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_track_timings(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_timings called, returning ");
    return _get_timings(track_id);
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_processor_timings(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_timings called, returning ");
    return _get_timings(processor_id);
}

ext::ControlStatus Controller::reset_all_timings()
{
    SUSHI_LOG_DEBUG("reset_all_timings called, returning ");
    _performance_timer->clear_all_timings();
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::reset_track_timings(int track_id)
{
    SUSHI_LOG_DEBUG("reset_track_timings called, returning ");
    auto success =_performance_timer->clear_timings_for_node(track_id);
    return success? ext::ControlStatus::OK : ext::ControlStatus::NOT_FOUND;
}

ext::ControlStatus Controller::reset_processor_timings(int processor_id)
{
    SUSHI_LOG_DEBUG("reset_processor_timings called, returning ");
    return reset_track_timings(processor_id);
}

std::pair<ext::ControlStatus, int> Controller::get_track_id(const std::string& track_name) const
{
    SUSHI_LOG_DEBUG("get_track_id called with track {}", track_name);
    return this->get_processor_id(track_name);
}

std::pair<ext::ControlStatus, ext::TrackInfo> Controller::get_track_info(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_info called with track {}", track_id);
    ext::TrackInfo info;
    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (static_cast<int>(track->id()) == track_id)
        {
            info.label = track->label();
            info.name = track->name();
            info.id = track->id();
            info.input_channels = track->input_channels();
            info.input_busses = track->input_busses();
            info.output_channels = track->output_channels();
            info.output_busses = track->output_busses();
            info.processor_count = 0 ; //static_cast<int>(track->process_chain().size());
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> Controller::get_track_processors(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_processors called for track: {}", track_id);
    const auto& tracks = _engine->processors_on_track(track_id);
    std::vector<ext::ProcessorInfo> infos;
    if (tracks.empty())
    {
        return {ext::ControlStatus::NOT_FOUND, infos};
    }
    for (const auto& processor : tracks)
    {
        ext::ProcessorInfo info;
        info.label = processor->label();
        info.name = processor->name();
        info.id = processor->id();
        info.parameter_count = processor->parameter_count();
        info.program_count = processor->supports_programs()? processor->program_count() : 0;
        infos.push_back(info);
    }
    return {ext::ControlStatus::OK, infos};
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> Controller::get_track_parameters(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_parameters called for track: {}", track_id);
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
                info.unit = param->unit();
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
    SUSHI_LOG_DEBUG("get_processor_id called with processor {}", processor_name);
    auto [status, id] = _engine->processor_id_from_name(processor_name);
    if (status == engine::EngineReturnStatus::OK)
    {
        return {ext::ControlStatus::OK, id};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::ProcessorInfo> Controller::get_processor_info(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_info called with processor {}", processor_id);
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
    SUSHI_LOG_DEBUG("get_processor_bypass_state called with processor {}", processor_id);
    auto processor = _engine->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, false};
    }
    return {ext::ControlStatus::OK, processor->bypassed()};
}

ext::ControlStatus Controller::set_processor_bypass_state(int processor_id, bool bypass_enabled)
{
    SUSHI_LOG_DEBUG("set_processor_bypass_state called with {} and processor {}", bypass_enabled, processor_id);
    auto processor = _engine->mutable_processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    processor->set_bypassed(bypass_enabled);
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, int> Controller::get_processor_current_program(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_current_program called with processor {}", processor_id);
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
    SUSHI_LOG_DEBUG("get_processor_current_program_name called with processor {}", processor_id);
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
    SUSHI_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
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
    SUSHI_LOG_DEBUG("get_processor_program_name called with processor {}", processor_id);
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

ext::ControlStatus Controller::set_processor_program(int processor_id, int program_id)
{
    SUSHI_LOG_DEBUG("set_processor_program called with processor {} and program {}", processor_id, program_id);
    auto event = new ProgramChangeEvent(static_cast<ObjectId>(processor_id), program_id, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>>
Controller::get_processor_parameters(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_parameters called with processor {}", processor_id);
    const auto proc = _engine->processor(processor_id);
    if (proc)
    {
        std::vector<ext::ParameterInfo> infos;
        const auto& params = proc->all_parameters();
        for (const auto& param : params)
        {
            ext::ParameterInfo info;
            info.label = param->label();
            info.name = param->name();
            info.unit = param->unit();
            info.id = param->id();
            info.type = ext::ParameterType::FLOAT;
            info.min_range = param->min_range();
            info.max_range = param->max_range();
            infos.push_back(info);
        }
        return {ext::ControlStatus::OK, infos};
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> Controller::get_parameter_id(int processor_id, const std::string& parameter_name) const
{
    SUSHI_LOG_DEBUG("get_parameter_id called with processor {} and parameter {}", processor_id, parameter_name);
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

std::pair<ext::ControlStatus, ext::ParameterInfo> Controller::get_parameter_info(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_info called with processor {} and parameter {}", processor_id, parameter_id);
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
            info.unit = descr->unit();
            info.type = to_external(descr->type());
            info.min_range = descr->min_range();
            info.max_range = descr->max_range();
            info.automatable =  descr->type() == ParameterType::FLOAT || // TODO - this might not be the way we eventually want it
                                descr->type() == ParameterType::INT   ||
                                descr->type() == ParameterType::BOOL;
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, float> Controller::get_parameter_value(int processor_id, int parameter_id) const
{
    SUSHI_LOG_DEBUG("get_parameter_value called with processor {} and parameter {}", processor_id, parameter_id);
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
    SUSHI_LOG_DEBUG("get_parameter_value_normalised called with processor {} and parameter {}", processor_id, parameter_id);
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
    SUSHI_LOG_DEBUG("get_parameter_value_as_string called with processor {} and parameter {}", processor_id, parameter_id);
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
    SUSHI_LOG_DEBUG("get_string_property_value called");
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, ""};
}

ext::ControlStatus Controller::set_parameter_value(int processor_id, int parameter_id, float value)
{
    SUSHI_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id, value);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_parameter_value_normalised(int processor_id, int parameter_id, float value)
{
    // TODO - this is exactly the same as set_parameter_value_now
    SUSHI_LOG_DEBUG("set_parameter_value called with processor {}, parameter {} and value {}", processor_id, parameter_id, value);
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          static_cast<ObjectId>(processor_id),
                                          static_cast<ObjectId>(parameter_id),
                                          value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_string_property_value(int /*processor_id*/, int /*parameter_id*/, const std::string& /*value*/)
{
    SUSHI_LOG_DEBUG("set_string_property_value called");
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::_get_timings(int node) const
{
    if (_performance_timer->enabled())
    {
        auto timings = _performance_timer->timings_for_node(node);
        if (timings.has_value())
        {
            return {ext::ControlStatus::OK, to_external(timings.value())};
        }
        return {ext::ControlStatus::NOT_FOUND, {0,0,0}};
    }
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, {0,0,0}};
}

}// namespace sushi