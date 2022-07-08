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
#include "library/processor_state.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

inline ext::ProcessorInfo to_external(const Processor* proc)
{
    return ext::ProcessorInfo{.id = static_cast<int>(proc->id()),
                              .label = proc->label(),
                              .name = proc->name(),
                              .parameter_count = proc->parameter_count(),
                              .program_count = proc->supports_programs()? proc->program_count() : 0};
}

inline ext::TrackInfo to_external(const Track* track, std::vector<int> proc_ids)
{
    return ext::TrackInfo{.id = static_cast<int>(track->id()),
                          .label = track->label(),
                          .name = track->name(),
                          .channels = track->input_channels(),
                          .buses = track->buses(),
                          .type = to_external(track->type()),
                          .processors = std::move(proc_ids)};
}

AudioGraphController::AudioGraphController(BaseEngine* engine) : _engine(engine),
                                                                 _event_dispatcher(engine->event_dispatcher()),
                                                                 _processors(engine->processor_container())
{}

std::vector<ext::ProcessorInfo> AudioGraphController::get_all_processors() const
{
    SUSHI_LOG_DEBUG("get_all_processors called");
    auto processors = _processors->all_processors();
    std::vector<ext::ProcessorInfo> returns;
    returns.reserve(processors.size());

    for (const auto& p : processors)
    {
        returns.push_back(to_external(p.get()));
    }
    return returns;
}

std::vector<ext::TrackInfo> AudioGraphController::get_all_tracks() const
{
    SUSHI_LOG_DEBUG("get_tracks called");
    auto tracks = _processors->all_tracks();
    std::vector<ext::TrackInfo> returns;
    returns.reserve(tracks.size());

    for (const auto& t : tracks)
    {
        returns.push_back(to_external(t.get(), _get_processor_ids(t->id())));
    }
    return returns;
}

std::pair<ext::ControlStatus, int> AudioGraphController::get_track_id(const std::string& track_name) const
{
    SUSHI_LOG_DEBUG("get_track_id called with track {}", track_name);

    auto track = _processors->track(track_name);
    if (track)
    {
        return {ext::ControlStatus::OK, track->id()};
    }
    return {ext::ControlStatus::NOT_FOUND, 0};
}

std::pair<ext::ControlStatus, ext::TrackInfo> AudioGraphController::get_track_info(int track_id) const
{
    SUSHI_LOG_DEBUG("get_track_info called with track {}", track_id);
    ext::TrackInfo info;
    const auto& tracks = _processors->all_tracks();
    auto track = std::find_if(tracks.cbegin(), tracks.cend(),
                              [&] (const auto& t) {return t->id() == static_cast<ObjectId>(track_id);});

    if (track != tracks.cend())
    {
        return {ext::ControlStatus::OK, to_external((*track).get(), _get_processor_ids((*track)->id()))};
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
        infos.push_back(to_external(processor.get()));
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

    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor)
    {
        return {ext::ControlStatus::OK, to_external(processor.get())};
    }
    return {ext::ControlStatus::NOT_FOUND, ext::ProcessorInfo()};
}

std::pair<ext::ControlStatus, bool> AudioGraphController::get_processor_bypass_state(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_bypass_state called with processor {}", processor_id);
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor)
    {
        return {ext::ControlStatus::OK, processor->bypassed()};
    }
    return {ext::ControlStatus::NOT_FOUND, false};
}

std::pair<ext::ControlStatus, ext::ProcessorState> AudioGraphController::get_processor_state(int processor_id) const
{
    SUSHI_LOG_DEBUG("get_processor_state called with processor {}", processor_id);
    ext::ProcessorState state;
    auto processor = _processors->processor(static_cast<ObjectId>(processor_id));
    if (processor)
    {
        state.bypassed = processor->bypassed();
        if (processor->supports_programs())
        {
            state.program = processor->current_program();
        }
        auto params = processor->all_parameters();
        for (const auto& param : params)
        {
            if (param->type() == sushi::ParameterType::STRING)
            {
                state.properties.push_back({static_cast<int>(param->id()), processor->property_value(param->id()).second});
            }
            else
            {
                state.parameters.push_back({static_cast<int>(param->id()), processor->parameter_value(param->id()).second});
            }
        }
        return {ext::ControlStatus::OK, state};
    }
    return {ext::ControlStatus::NOT_FOUND, state};
}

ext::ControlStatus AudioGraphController::set_processor_state(int processor_id, const ext::ProcessorState& state)
{
    SUSHI_LOG_DEBUG("set_processor_state called with processor id {}", processor_id);
    auto internal_state = std::make_unique<sushi::ProcessorState>();
    to_internal(internal_state.get(), &state);

    // Capture everything by copy, except internal_state which is captured by move.
    auto lambda = [=, state = std::move(internal_state)] () -> int
    {
        bool realtime = _engine->realtime();

        auto processor = _processors->mutable_processor(static_cast<ObjectId>(processor_id));
        if (processor)
        {
            SUSHI_LOG_DEBUG("Setting state on processor {} with realtime {}", processor->name(), realtime? "enabled" : "disabled");
            processor->set_state(state.get(), realtime);
            return EventStatus::HANDLED_OK;
        }
        SUSHI_LOG_ERROR("Processor {} not found", processor_id);
        return EventStatus::ERROR;
    };

    auto event = new LambdaEvent(std::move(lambda), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::set_processor_bypass_state(int processor_id, bool bypass_enabled)
{
    SUSHI_LOG_DEBUG("set_processor_bypass_state called with {} and processor {}", bypass_enabled, processor_id);
    auto processor = _processors->mutable_processor(static_cast<ObjectId>(processor_id));
    if (processor)
    {
        processor->set_bypassed(bypass_enabled);
        return ext::ControlStatus::OK;
    }
    return ext::ControlStatus::NOT_FOUND;
}

ext::ControlStatus AudioGraphController::create_track(const std::string& name, int channels)
{
    SUSHI_LOG_DEBUG("create_track called with name {} and {} channels", name, channels);

    auto lambda = [=] () -> int
    {
        auto [status, track_id] = _engine->create_track(name, channels);
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::create_multibus_track(const std::string& name, int buses)
{
    SUSHI_LOG_DEBUG("create_multibus_track called with name {} and {} buses ", name, buses);
    auto lambda = [=] () -> int
    {
        auto [status, track_id] = _engine->create_multibus_track(name, buses);
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::create_pre_track(const std::string& name)
{
    SUSHI_LOG_DEBUG("create_pre_track called with name {}", name);
    auto lambda = [=] () -> int
    {
        auto [status, track_id] = _engine->create_pre_track(name);
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::create_post_track(const std::string& name)
{
    SUSHI_LOG_DEBUG("create_post_track called with name {}", name);
    auto lambda = [=] () -> int
    {
        auto [status, track_id] = _engine->create_post_track(name);
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::move_processor_on_track(int processor_id,
                                                                 int source_track_id,
                                                                 int dest_track_id,
                                                                 std::optional<int> before_processor_id)
{
    SUSHI_LOG_DEBUG("move_processor_on_track called with processor id {}, source track id and {} dest track id {}",
                    processor_id, source_track_id, dest_track_id);
    auto lambda = [=] () -> int
    {
        auto plugin_order = _processors->processors_on_track(source_track_id);

        if (_processors->processor_exists(dest_track_id) == false ||
            plugin_order.empty())
        {
            // Normally controllers aren't supposed to do this kind of pre-check as is results in
            // double look ups of Processor and Track objects. But given the amount of work needed to
            // restore a failed insertion, this is justified in this case
            SUSHI_LOG_ERROR("Processor or dest track not found");
            return EventStatus::ERROR;
        }

        auto status = _engine->remove_plugin_from_track(processor_id, source_track_id);
        if (status != EngineReturnStatus::OK)
        {
            return EventStatus::HANDLED_OK;
        }

        status = _engine->add_plugin_to_track(processor_id, dest_track_id, before_processor_id);
        if (status != engine::EngineReturnStatus::OK)
        {
            SUSHI_LOG_ERROR("Failed to move processor {} from track {} to track {} with error {}, reverting",
                             processor_id, source_track_id, dest_track_id, static_cast<int>(status));

            // If the insertion operation failed, we must put the processor back in the source track
            std::optional<ObjectId> position(std::nullopt);
            if (plugin_order.back()->id() != static_cast<ObjectId>(processor_id))
            {
                // plugin position should be the id of the plugin that originally came after
                auto p = std::find_if(plugin_order.cbegin(), plugin_order.cend(),
                                      [&] (const auto& p) {return p->id() == static_cast<ObjectId>(processor_id);});
                position = std::optional<ObjectId>((*p++)->id());
            }

            [[maybe_unused]] auto replace_status = _engine->add_plugin_to_track(processor_id, source_track_id, position);
            SUSHI_LOG_WARNING_IF(replace_status != engine::EngineReturnStatus::OK,
                                 "Failed to replace processor {} on track {}", processor_id, source_track_id)

        }
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
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
    SUSHI_LOG_DEBUG("create_processor_on_track called with name {}, uid {} from {} on track {}",
                                                                    name, uid, file, track_id);
    auto lambda = [=] () -> int
    {
        PluginInfo plugin_info;
        plugin_info.uid = uid;
        plugin_info.path = file;
        plugin_info.type = to_internal(type);

        auto [status, plugin_id] = _engine->create_processor(plugin_info, name);
        if (status != EngineReturnStatus::OK)
        {
            return EventStatus::ERROR;
        }

        SUSHI_LOG_DEBUG("Adding plugin {} to track {}", name, track_id);
        status = _engine->add_plugin_to_track(plugin_id, track_id, before_processor_id);
        if (status != engine::EngineReturnStatus::OK)
        {
            SUSHI_LOG_ERROR("Failed to load plugin {} to track {}, destroying plugin", plugin_id, track_id);
            _engine->delete_plugin(plugin_id);
        }

        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::delete_processor_from_track(int processor_id, int track_id)
{
    SUSHI_LOG_DEBUG("delete processor_from_track called with processor id {} and track id {}",
                    processor_id, track_id);
    auto lambda = [=] () -> int
    {
        auto status = _engine->remove_plugin_from_track(processor_id, track_id);
        if (status == engine::EngineReturnStatus::OK)
        {
            status = _engine->delete_plugin(processor_id);
        }
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus AudioGraphController::delete_track(int track_id)
{
    SUSHI_LOG_DEBUG("delete_track called with id {}", track_id);
    auto lambda = [=] () -> int
    {
        auto track = _processors->track(track_id);
        if (track == nullptr)
        {
            return EventStatus::ERROR;
        }
        auto processors = _processors->processors_on_track(track_id);

        // Remove processors starting with the last, more efficient
        for (auto p = processors.rbegin(); p != processors.rend(); ++p)
        {
            SUSHI_LOG_DEBUG("Removing plugin {} from track: {}", (*p)->name(), track->name());
            auto status = _engine->remove_plugin_from_track((*p)->id(), track_id);
            if (status == engine::EngineReturnStatus::OK)
            {
                status = _engine->delete_plugin((*p)->id());
            }
            if (status != engine::EngineReturnStatus::OK)
            {
                SUSHI_LOG_ERROR("Failed to remove plugin {} from track {}", (*p)->name(), track->name());
            }
        }
        auto status = _engine->delete_track(track_id);
        return status == EngineReturnStatus::OK? EventStatus::HANDLED_OK : EventStatus::ERROR;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
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


} // namespace controller_impl
} // namespace engine
} // namespace sushi
