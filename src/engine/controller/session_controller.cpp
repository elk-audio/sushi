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

#include "spdlog/fmt/bundled/format.h"

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


SessionController::SessionController(BaseEngine* engine,
                                     midi_dispatcher::MidiDispatcher* midi_dispatcher) : _event_dispatcher(engine->event_dispatcher()),
                                                                                         _engine(engine),
                                                                                         _midi_dispatcher(midi_dispatcher),
                                                                                         _processors(engine->processor_container()),
                                                                                         _osc_frontend(nullptr)
{}

void SessionController::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

ext::SessionState SessionController::save_session() const
{
    ext::SessionState session;

    //session.save_date = fmt::format("{}", std::chrono::system_clock::now().time_since_epoch());
    session.sushi_info = _save_build_info();
    session.osc_state = _save_osc_state();
    session.midi_state = _save_midi_state();
    session.engine_state = _save_engine_state();
    session.tracks = _save_tracks();

    return session;
}

ext::ControlStatus SessionController::restore_session(const ext::SessionState& /*state*/)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
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
    return ext::OscState();
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
    state.input_clip_detection = false; // Todo: enable functions to query this
    state.output_clip_detection = false; // Todo: enable functions to query this
    state.master_limiter = false; // Todo: enable functions to query this
    state.audio_inputs = _engine->audio_input_channels();
    state.audio_outputs = _engine->audio_output_channels();
    state.cv_inputs = 0;
    state.cv_outputs = 0;
    for (const auto& con : _engine->audio_input_connections())
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            state.input_connections.push_back(to_external(con, track->name()));
        }
    }
    for (const auto& con : _engine->audio_output_connections())
    {
        auto track = _processors->track(con.track);
        if (track)
        {
            state.output_connections.push_back(to_external(con, track->name()));
        }
    }
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
        state.input_channels = track->input_channels();
        state.output_channels = track->output_channels();
        state.input_busses = track->input_busses();
        state.output_busses = track->output_busses();

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


} // namespace controller_impl
} // namespace engine
} // namespace sushi
