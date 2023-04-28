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

#ifndef SUSHI_SESSION_CONTROLLER_H
#define SUSHI_SESSION_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"
#include "control_frontends/osc_frontend.h"
#include "audio_frontends/base_audio_frontend.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class SessionController : public ext::SessionController
{
public:
    SessionController(BaseEngine* engine,
                      midi_dispatcher::MidiDispatcher* midi_dispatcher,
                      audio_frontend::BaseAudioFrontend* audio_frontend);

    ~SessionController() override = default;

    void set_osc_frontend(control_frontend::OSCFrontend* osc_frontend);

    ext::SessionState save_session() const override;

    ext::ControlStatus restore_session(const ext::SessionState& state) override;

private:
    ext::SushiBuildInfo _save_build_info() const;
    ext::OscState       _save_osc_state() const;
    ext::MidiState      _save_midi_state() const;
    ext::EngineState    _save_engine_state() const;
    std::vector<ext::TrackState> _save_tracks() const;
    ext::PluginClass    _save_plugin(const sushi::Processor* plugin) const;

    bool _check_state(const ext::SessionState& state) const;
    void _restore_tracks(std::vector<ext::TrackState> tracks);
    void _restore_plugin_states(std::vector<ext::TrackState> tracks);
    void _restore_plugin(ext::PluginClass plugin, sushi::engine::Track* track);
    void _restore_engine(ext::EngineState& state);
    void _restore_midi(ext::MidiState& state);
    void _restore_osc(ext::OscState& state);
    void _clear_all_tracks();

    dispatcher::BaseEventDispatcher*    _event_dispatcher;
    engine::BaseEngine*                 _engine;
    midi_dispatcher::MidiDispatcher*    _midi_dispatcher;
    audio_frontend::BaseAudioFrontend*  _audio_frontend;
    const BaseProcessorContainer*       _processors;
    control_frontend::OSCFrontend*      _osc_frontend;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_SESSION_CONTROLLER_H
