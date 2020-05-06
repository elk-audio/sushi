/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "audio_graph_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

inline sushi::AddProcessorToTrackEvent::ProcessorType to_event_type(ext::PluginType type)
{
    switch (type)
    {
        case ext::PluginType::INTERNAL: return sushi::AddProcessorToTrackEvent::ProcessorType::INTERNAL;
        case ext::PluginType::VST2X:    return sushi::AddProcessorToTrackEvent::ProcessorType::VST2X;
        case ext::PluginType::VST3X:    return sushi::AddProcessorToTrackEvent::ProcessorType::VST3X;
        case ext::PluginType::LV2:      return sushi::AddProcessorToTrackEvent::ProcessorType::LV2;
        default:                        return sushi::AddProcessorToTrackEvent::ProcessorType::INTERNAL;
    }
}

AudioGraphController::AudioGraphController(BaseEngine* engine) : _engine(engine),
                                                                 _event_dispatcher(engine->event_dispatcher()),
                                                                 _processors(engine->processor_container())
{}

std::vector<ext::ProcessorInfo> AudioGraphController::get_all_processors() const
{
    return std::vector<ext::ProcessorInfo>();
}

std::vector<ext::TrackInfo> AudioGraphController::get_all_tracks() const
{
    SUSHI_LOG_DEBUG("get_tracks called");
    auto tracks = _processors->all_tracks();
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
        info.processors = _get_processor_ids(t->id());
        returns.push_back(info);
    }
    return returns;
}

std::pair<ext::ControlStatus, int> AudioGraphController::get_track_id(const std::string& track_name) const
{
    SUSHI_LOG_DEBUG("get_track_id called with track {}", track_name);
    return this->get_processor_id(track_name);
}

std::pair<ext::ControlStatus, ext::TrackInfo> AudioGraphController::get_track_info(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_info called with track {}", track_id);
    ext::TrackInfo info;
    const auto& tracks = _processors->all_tracks();
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
            info.processors = _get_processor_ids(track->id());
            return {ext::ControlStatus::OK, info};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, info};
}

std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> AudioGraphController::get_track_processors(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_processors called for track: {}", track_id);
    const auto& tracks = _processors->processors_on_track(track_id);
    std::vector<ext::ProcessorInfo> infos;
    if (tracks.empty() && _processors->processor_exists(track_id) == false)
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

std::pair<ext::ControlStatus, int> AudioGraphController::get_processor_id(const std::string& processor_name) const
{
    SUSHI_LOG_DEBUG("get_processor_id called with processor {}", processor_name);
    auto processor = _processors->processor(processor_name);
    if (processor)
    {
        return {ext::ControlStatus::OK, processor->id()};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::ProcessorInfo> AudioGraphController::get_processor_info(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_info called with processor {}", processor_id);
    ext::ProcessorInfo info;
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
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

std::pair<ext::ControlStatus, bool> AudioGraphController::get_processor_bypass_state(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_bypass_state called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return {ext::ControlStatus::NOT_FOUND, false};
    }
    return {ext::ControlStatus::OK, processor->bypassed()};
}

ext::ControlStatus AudioGraphController::set_processor_bypass_state(int processor_id, bool bypass_enabled)
{
    SUSHI_LOG_DEBUG("set_processor_bypass_state called with {} and processor {}", bypass_enabled, processor_id);
    auto processor = _processors->mutable_processor(static_cast<ObjectId>(processor_id));
    if (processor == nullptr)
    {
        return ext::ControlStatus::NOT_FOUND;
    }
    processor->set_bypassed(bypass_enabled);
    return ext::ControlStatus::OK;
}

// TODO - Remove when create track functions are complete
#pragma GCC diagnostic ignored "-Wunused-parameter"

ext::ControlStatus AudioGraphController::create_track(const std::string& name, int channels)
{
    // TODO - pass the channel count along and not the busses
    auto event = new AddTrackEvent(name, 2, 0, 0, IMMEDIATE_PROCESS);
    event->set_completion_cb(AudioGraphController::completion_callback, this);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::create_multibus_track(const std::string& name,
                                                               int input_busses,
                                                               int output_channels)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

#pragma GCC diagnostic pop

ext::ControlStatus AudioGraphController::move_processor_on_track(int processor_id,
                                                                 int source_track_id,
                                                                 int dest_track_id,
                                                                 std::optional<int> before_processor_id)
{
    MoveProcessorEvent* event;
    event = new MoveProcessorEvent(ObjectId(processor_id),
                                   ObjectId(source_track_id),
                                   ObjectId(dest_track_id),
                                   before_processor_id,
                                   IMMEDIATE_PROCESS);
    event->set_completion_cb(AudioGraphController::completion_callback, this);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::create_processor_on_track(const std::string& name,
                                                                   const std::string& uid,
                                                                   const std::string& file,
                                                                   ext::PluginType type,
                                                                   int track_id,
                                                                   std::optional<int> before_processor_id)
{
    auto event = new AddProcessorToTrackEvent(name,
                                              uid,
                                              file,
                                              to_event_type(type),
                                              ObjectId(track_id),
                                              before_processor_id,
                                              IMMEDIATE_PROCESS);

    event->set_completion_cb(AudioGraphController::completion_callback, this);
    _event_dispatcher->post_event(event);

    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::delete_processor_from_track(int processor_id, int track_id)
{
    auto event = new RemoveProcessorEvent(ObjectId(processor_id), ObjectId(track_id), IMMEDIATE_PROCESS);
    event->set_completion_cb(AudioGraphController::completion_callback, this);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::delete_track(int track_id)
{
    auto event = new RemoveTrackEvent(ObjectId(track_id), IMMEDIATE_PROCESS);
    event->set_completion_cb(AudioGraphController::completion_callback, this);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

void AudioGraphController::completion_callback(void*arg, Event*event, int status)
{
    reinterpret_cast<AudioGraphController*>(arg)->_completion_callback(event, status);
}

std::vector<int> AudioGraphController::_get_processor_ids(int track_id) const
{
    std::vector<int> ids;
    auto processors = _processors->processors_on_track(ObjectId(track_id));
    ids.reserve(processors.size());
    for (const auto& p : processors)
    {
        ids.push_back(p->id());
    }
    return ids;
}

void AudioGraphController::_completion_callback([[maybe_unused]] Event* event, int status)
{
    if (status == EventStatus::HANDLED_OK)
    {
        SUSHI_LOG_INFO("Event {}, handled OK", event->id());
    }
    else
    {
        SUSHI_LOG_WARNING("Event {} returned with error code: ", event->id(), status);
    }
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi
