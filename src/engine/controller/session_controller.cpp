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
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <ctime>

#include "spdlog/fmt/bundled/chrono.h"

#include "library/constants.h"
#include "compile_time_settings.h"
#include "session_controller.h"
#include "controller_common.h"

#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

inline ext::TrackAudioConnectionState to_external(const ::sushi::AudioConnection& con,
                                                  const std::string& track_name)
{
    ext::TrackAudioConnectionState ext_con;
    ext_con.track = track_name;
    ext_con.track_channel = con.track_channel;
    ext_con.engine_channel = con.engine_channel;
    return ext_con;
}

inline ext::MidiKbdConnectionState to_external(const ::sushi::midi_dispatcher::KbdInputConnection& con,
                                               const std::string& track_name)
{
    ext::MidiKbdConnectionState ext_con;
    ext_con.track = track_name;
    ext_con.channel = to_external_midi_channel(con.channel);
    ext_con.port = con.port;
    ext_con.raw_midi = con.raw_midi;
    return ext_con;
}

inline ext::MidiKbdConnectionState to_external(const ::sushi::midi_dispatcher::KbdOutputConnection& con,
                                               const std::string& track_name)
{
    ext::MidiKbdConnectionState ext_con;
    ext_con.track = track_name;
    ext_con.channel = to_external_midi_channel(con.channel);
    ext_con.port = con.port;
    ext_con.raw_midi = false;
    return ext_con;
}

inline ext::MidiCCConnectionState to_external(const ::sushi::midi_dispatcher::CCInputConnection& con,
                                              const std::string& track_name)
{
    ext::MidiCCConnectionState ext_con;
    ext_con.processor = track_name;
    ext_con.channel = to_external_midi_channel(con.channel);
    ext_con.port = con.port;
    ext_con.parameter_id = con.input_connection.parameter;
    ext_con.cc_number = con.cc;
    ext_con.min_range = con.input_connection.min_range;
    ext_con.max_range = con.input_connection.max_range;
    ext_con.relative_mode = con.input_connection.relative;
    return ext_con;
}

inline ext::MidiPCConnectionState to_external(const ::sushi::midi_dispatcher::PCInputConnection& con,
                                              const std::string& track_name)
{
    ext::MidiPCConnectionState ext_con;
    ext_con.processor = track_name;
    ext_con.channel = to_external_midi_channel(con.channel);
    ext_con.port = con.port;
    return ext_con;
}

inline void to_internal(sushi::control_frontend::OscState& dest, const ext::OscState& src)
{
    dest.set_auto_enable_outputs(src.enable_all_processor_outputs);
    for (const auto& output : src.enabled_processor_outputs)
    {
        dest.add_enabled_outputs(std::string(output.processor), {output.parameter_ids.begin(), output.parameter_ids.end()});
    }
}

inline void to_external(ext::OscState& dest, const sushi::control_frontend::OscState& src)
{
    dest.enable_all_processor_outputs = src.auto_enable_outputs();
    for (const auto& output : src.enabled_outputs())
    {
        dest.enabled_processor_outputs.push_back({output.first, {output.second.begin(), output.second.end()}});
    }
}

SessionController::SessionController(BaseEngine* engine,
                                     midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                     audio_frontend::BaseAudioFrontend* audio_frontend) : _event_dispatcher(engine->event_dispatcher()),
                                                                                          _engine(engine),
                                                                                          _midi_dispatcher(midi_dispatcher),
                                                                                          _audio_frontend(audio_frontend),
                                                                                          _processors(engine->processor_container()),
                                                                                          _osc_frontend(nullptr)
{}

void SessionController::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

ext::SessionState SessionController::save_session() const
{
    SUSHI_LOG_DEBUG("save_session called");

    ext::SessionState session;
    auto date = time(nullptr);
    session.save_date = fmt::format("{:%Y-%m-%d %H:%M}", fmt::localtime(date));
    session.sushi_info = _save_build_info();
    session.osc_state = _save_osc_state();
    session.midi_state = _save_midi_state();
    session.engine_state = _save_engine_state();
    session.tracks = _save_tracks();

    return session;
}

ext::ControlStatus SessionController::restore_session(const ext::SessionState& state)
{
    SUSHI_LOG_DEBUG("restore_session called");
    if (_check_state(state) == false)
    {
        return ext::ControlStatus::INVALID_ARGUMENTS;
    }
    auto new_session = std::make_unique<ext::SessionState>(state);

    auto lambda = [&, state = std::move(new_session)] () -> int
    {
        bool realtime = _engine->realtime();
        if (realtime)
        {
            SUSHI_LOG_DEBUG("Pausing engine");
            _audio_frontend->pause(true);
        }
        _clear_all_tracks();
        _restore_tracks(state->tracks);
        _restore_plugin_states(state->tracks);
        _restore_engine(state->engine_state);
        _restore_midi(state->midi_state);
        _restore_osc(state->osc_state);

        if (realtime)
        {
            SUSHI_LOG_DEBUG("Un-Pausing engine");
            _audio_frontend->pause(false);
        }
        return EventStatus::HANDLED_OK;
    };

    auto event = new LambdaEvent(std::move(lambda), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::SushiBuildInfo SessionController::_save_build_info() const
{
    ext::SushiBuildInfo info;
    for(auto& option : ::CompileTimeSettings::enabled_build_options)
    {
        info.build_options.emplace_back(option);
    }

    info.version = CompileTimeSettings::sushi_version;
    info.audio_buffer_size = AUDIO_CHUNK_SIZE;
    info.commit_hash = SUSHI_GIT_COMMIT_HASH;
    info.build_date = SUSHI_BUILD_TIMESTAMP;
    return info;
}

ext::OscState SessionController::_save_osc_state() const
{
    ext::OscState ext_state;
    if (_osc_frontend)
    {
        auto state = _osc_frontend->save_state();
        to_external(ext_state, state);
    }
    return ext_state;
}

ext::MidiState SessionController::_save_midi_state() const
{
    ext::MidiState state;

    state.inputs = _midi_dispatcher->get_midi_inputs();
    state.outputs = _midi_dispatcher->get_midi_outputs();

    for (const auto& con : _midi_dispatcher->get_all_kb_input_connections())
    {
        auto track = _processors->track(con.input_connection.target);
        if (track)
        {
            state.kbd_input_connections.push_back(to_external(con, track->name()));
        }
    }
    for (const auto& con : _midi_dispatcher->get_all_kb_output_connections())
    {
        auto track = _processors->track(con.track_id);
        if (track)
        {
            state.kbd_output_connections.push_back(to_external(con, track->name()));
        }
    }
    for (const auto& con : _midi_dispatcher->get_all_cc_input_connections())
    {
        auto processor = _processors->processor(con.input_connection.target);
        if (processor)
        {
            state.cc_connections.push_back(to_external(con, processor->name()));
        }
    }
    for (const auto& con : _midi_dispatcher->get_all_pc_input_connections())
    {
        auto processor = _processors->processor(con.processor_id);
        if (processor)
        {
            state.pc_connections.push_back(to_external(con, processor->name()));
        }
    }
    for (int port = 0; port < _midi_dispatcher->get_midi_outputs(); ++port)
    {
        if (_midi_dispatcher->midi_clock_enabled(port))
        {
            state.enabled_clock_outputs.push_back(port);
        }
    }
    return state;
}

ext::EngineState SessionController::_save_engine_state() const
{
    ext::EngineState state;
    auto transport = _engine->transport();

    state.sample_rate = _engine->sample_rate();
    state.tempo = transport->current_tempo();
    state.playing_mode = to_external(transport->playing_mode());
    state.sync_mode = to_external(transport->sync_mode());
    state.time_signature = to_external(transport->time_signature());
    state.input_clip_detection = _engine->input_clip_detection();
    state.output_clip_detection = _engine->output_clip_detection();
    state.master_limiter = _engine->master_limiter();
    state.used_audio_inputs = _engine->audio_input_channels();
    state.used_audio_outputs = _engine->audio_output_channels();

    int audio_inputs = 0;
    for (const auto& con : _engine->audio_input_connections())
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            state.input_connections.push_back(to_external(con, track->name()));
            audio_inputs = std::max(audio_inputs, con.engine_channel);
        }
    }
    // Store the minimum number of audio channels required to restore the session
    state.used_audio_inputs = audio_inputs;

    int audio_outputs = 0;
    for (const auto& con : _engine->audio_output_connections())
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            state.output_connections.push_back(to_external(con, track->name()));
            audio_outputs = std::max(audio_outputs, con.engine_channel);
        }
    }
    state.used_audio_outputs = audio_outputs;

    return state;
}

std::vector<ext::TrackState> SessionController::_save_tracks() const
{
    std::vector<ext::TrackState> tracks;
    for (const auto& track : _processors->all_tracks())
    {
        ext::TrackState state;
        auto track_state = track->save_state();
        to_external(&state.track_state, &track_state);
        state.name = track->name();
        state.label = track->label();
        state.channels = track->input_channels();
        state.buses = track->buses();
        state.type = to_external(track->type());

        for (const auto& plugin : _processors->processors_on_track(track->id()))
        {
            state.processors.push_back(_save_plugin(plugin.get()));
        }

        tracks.push_back(std::move(state));
    }
    return tracks;
}

ext::PluginClass SessionController::_save_plugin(const sushi::Processor* plugin) const
{
    ext::PluginClass plugin_class;

    auto info = plugin->info();
    plugin_class.name = plugin->name();
    plugin_class.label = plugin->label();
    plugin_class.uid = info.uid;
    plugin_class.path = info.path;
    plugin_class.type = to_external(info.type);

    auto plugin_state = plugin->save_state();
    to_external(&plugin_class.state, &plugin_state);

    return plugin_class;
}

bool SessionController::_check_state(const ext::SessionState& state) const
{
    // Todo: check if state was saved with a newer/older version of sushi.
    if (state.engine_state.used_audio_inputs > _engine->audio_input_channels() ||
        state.engine_state.used_audio_outputs > _engine->audio_output_channels())
    {
        SUSHI_LOG_ERROR("Audio engine doesn't have enough audio channels to restore saved session");
        return false;
    }
    if (state.midi_state.inputs > _midi_dispatcher->get_midi_inputs() ||
        state.midi_state.outputs > _midi_dispatcher->get_midi_outputs())
    {
        SUSHI_LOG_ERROR("Not enough midi inputs or outputs to restore saved session");
        return false;
    }
    return true;
}

void SessionController::_restore_tracks(std::vector<ext::TrackState> tracks)
{
    for (auto& track : tracks)
    {
        EngineReturnStatus status;
        ObjectId track_id;

        switch (track.type)
        {
            case ext::TrackType::PRE:
                std::tie(status, track_id) = _engine->create_pre_track(track.name);
                break;

            case ext::TrackType::POST:
                std::tie(status, track_id) = _engine->create_post_track(track.name);
                break;

            case ext::TrackType::REGULAR:
                if (track.buses > 1)
                {
                    std::tie(status, track_id) = _engine->create_multibus_track(track.name, track.buses);
                }
                else
                {
                    std::tie(status, track_id) = _engine->create_track(track.name, track.channels);
                }
        }

        auto track_instance = _processors->mutable_track(track_id);

        if (status != EngineReturnStatus::OK || !track_instance)
        {
            SUSHI_LOG_ERROR("Failed to restore track {} with error {}", track.name, static_cast<int>(status));
            continue;
        }

        for (auto& plugin : track.processors)
        {
            _restore_plugin(plugin, track_instance.get());
        }
    }
}

void SessionController::_restore_plugin_states(std::vector<ext::TrackState> tracks)
{
    for (auto& track : tracks)
    {
        auto track_instance = _processors->mutable_track(track.name);
        if (!track_instance)
        {
            SUSHI_LOG_ERROR("Track {} not found", track.name);
            continue;
        }

        sushi::ProcessorState state;
        to_internal(&state, &track.track_state);
        auto status = track_instance->set_state(&state, false);
        if (status != ProcessorReturnCode::OK)
        {
            SUSHI_LOG_ERROR("Failed to restore state to track {} with status {}", track.name, status);
        }

        for (auto& plugin : track.processors)
        {
            auto instance = _processors->mutable_processor(plugin.name);
            if (!instance)
            {
                SUSHI_LOG_ERROR("Plugin {} not found", plugin.name);
                continue;
            }
            to_internal(&state, &plugin.state);
            status = instance->set_state(&state, false);
            if (status != ProcessorReturnCode::OK)
            {
                SUSHI_LOG_ERROR("Failed to restore state to track {} with status {}", track.name, status);
            }
        }
    }
}

void SessionController::_restore_plugin(ext::PluginClass plugin, sushi::engine::Track* track)
{
    PluginInfo info {.uid = plugin.uid,
                     .path = plugin.path,
                     .type = to_internal(plugin.type)};
    auto [status, processor_id] = _engine->create_processor(info, plugin.name);
    auto instance = _processors->mutable_processor(processor_id);

    if (status == EngineReturnStatus::OK && instance)
    {
        instance->set_label(plugin.label);
        _engine->add_plugin_to_track(instance->id(), track->id());
    }
    else
    {
        SUSHI_LOG_ERROR("Failed to restore plugin {} on track {}", plugin.name, track->name());
    }
}

void SessionController::_restore_engine(ext::EngineState& state)
{
    if (_engine->sample_rate() != state.sample_rate)
    {
        SUSHI_LOG_WARNING("Saved session samplerate mismatch({}Hz vs {}Hz", _engine->sample_rate(), state.sample_rate);
    }
    _engine->set_tempo(state.tempo);
    _engine->set_tempo_sync_mode(to_internal(state.sync_mode));
    _engine->set_transport_mode(to_internal(state.playing_mode));
    _engine->set_time_signature(to_internal(state.time_signature));
    _engine->enable_input_clip_detection(state.input_clip_detection);
    _engine->enable_output_clip_detection(state.output_clip_detection);
    _engine->enable_master_limiter(state.master_limiter);

    [[maybe_unused]] EngineReturnStatus status;

    for (const auto& con : state.input_connections)
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            status = _engine->connect_audio_input_channel(con.engine_channel, con.track_channel, track->id());
            SUSHI_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Failed to connect channel {} of track {} to engine channel {}",
                               con.track_channel, con.engine_channel, con.track);
        }
    }

    for (const auto& con : state.output_connections)
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            status = _engine->connect_audio_output_channel(con.engine_channel, con.track_channel, track->id());
            SUSHI_LOG_ERROR_IF(status != EngineReturnStatus::OK, "Failed to connect engine channel {} from channel {} of track",
                               con.engine_channel, con.track_channel, con.track);
        }
    }
}

void SessionController::_restore_midi(ext::MidiState& state)
{
    [[maybe_unused]] midi_dispatcher::MidiDispatcherStatus status;
    for (const auto& con : state.kbd_input_connections)
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            if (con.raw_midi)
            {
                status = _midi_dispatcher->connect_raw_midi_to_track(con.port, track->id(), int_from_ext_midi_channel(con.channel));
            }
            else
            {
                status = _midi_dispatcher->connect_kb_to_track(con.port, track->id(), int_from_ext_midi_channel(con.channel));
            }
            SUSHI_LOG_ERROR_IF(status != midi_dispatcher::MidiDispatcherStatus::OK,
                               "Failed to connect midi kbd to track {}", track->name());
        }
    }

    for (const auto& con : state.kbd_output_connections)
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            status = _midi_dispatcher->connect_track_to_output(con.port, track->id(), int_from_ext_midi_channel(con.channel));
            SUSHI_LOG_ERROR_IF(status != midi_dispatcher::MidiDispatcherStatus::OK,
                               "Failed to connect midi kbd from track {} to output", track->name());
        }
    }

    for (const auto& con : state.cc_connections)
    {
        auto processor = _processors->processor(con.processor);
        if (processor)
        {
            status = _midi_dispatcher->connect_cc_to_parameter(con.port, processor->id(), con.parameter_id,
                                                               con.cc_number, con.min_range, con.max_range,
                                                               con.relative_mode, int_from_ext_midi_channel(con.channel));
            SUSHI_LOG_ERROR_IF(status != midi_dispatcher::MidiDispatcherStatus::OK,
                               "Failed to connect midi cc to parameter {} of processor {}", con.parameter_id, processor->id());
        }
    }

    for (const auto& con : state.pc_connections)
    {
        auto processor = _processors->processor(con.processor);
        if (processor)
        {
            status = _midi_dispatcher->connect_pc_to_processor(con.port, processor->id(), int_from_ext_midi_channel(con.channel));
            SUSHI_LOG_ERROR_IF(status != midi_dispatcher::MidiDispatcherStatus::OK,
                               "Failed to connect mid program change to processor {}", processor->name());
        }
    }

    for (int port = 0; port < _midi_dispatcher->get_midi_outputs(); ++port)
    {
        _midi_dispatcher->enable_midi_clock(false, port);
    }
    for (auto port : state.enabled_clock_outputs)
    {
        status = _midi_dispatcher->enable_midi_clock(true, port);
    }
}

void SessionController::_restore_osc(ext::OscState& state)
{
    if (_osc_frontend)
    {
        sushi::control_frontend::OscState internal_state;
        to_internal(internal_state, state);
        _osc_frontend->set_state(internal_state);
    }
}

void SessionController::_clear_all_tracks()
{
    auto tracks = _processors->all_tracks();
    for (const auto& track : tracks)
    {
        auto processors = _processors->processors_on_track(track->id());
        for (const auto& processor : processors)
        {
            _engine->remove_plugin_from_track(processor->id(), track->id());
            _engine->delete_plugin(processor->id());
        }
        _engine->delete_track(track->id());
    }
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi
