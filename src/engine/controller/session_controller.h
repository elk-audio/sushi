/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Implementation of external control interface for sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_SESSION_CONTROLLER_H
#define SUSHI_SESSION_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"
#include "control_frontends/osc_frontend.h"
#include "audio_frontends/base_audio_frontend.h"

namespace sushi::internal::engine::controller_impl {

class Accessor;

class SessionController : public control::SessionController
{
public:
    SessionController(BaseEngine* engine,
                      midi_dispatcher::MidiDispatcher* midi_dispatcher,
                      audio_frontend::BaseAudioFrontend* audio_frontend);

    ~SessionController() override = default;

    void set_osc_frontend(control_frontend::OSCFrontend* osc_frontend);

    [[nodiscard]] control::SessionState save_session() const override;

    control::ControlStatus restore_session(const control::SessionState& state) override;

private:
    friend Accessor;

    [[nodiscard]] control::SushiBuildInfo _save_build_info() const;
    [[nodiscard]] control::OscState       _save_osc_state() const;
    [[nodiscard]] control::MidiState      _save_midi_state() const;
    [[nodiscard]] control::EngineState    _save_engine_state() const;
    [[nodiscard]] std::vector<control::TrackState> _save_tracks() const;
    [[nodiscard]] control::PluginClass    _save_plugin(const sushi::internal::Processor* plugin) const;

    [[nodiscard]] bool _check_state(const control::SessionState& state) const;

    void _restore_tracks(std::vector<control::TrackState> tracks);
    void _restore_plugin_states(std::vector<control::TrackState> tracks);
    void _restore_plugin(control::PluginClass plugin, sushi::internal::engine::Track* track);
    void _restore_engine(control::EngineState& state);
    void _restore_midi(control::MidiState& state);
    void _restore_osc(control::OscState& state);
    void _clear_all_tracks();

    dispatcher::BaseEventDispatcher*    _event_dispatcher;
    engine::BaseEngine*                 _engine;
    midi_dispatcher::MidiDispatcher*    _midi_dispatcher;
    audio_frontend::BaseAudioFrontend*  _audio_frontend;
    const BaseProcessorContainer*       _processors;
    control_frontend::OSCFrontend*      _osc_frontend;
};

class Accessor
{
public:
    explicit Accessor(SessionController& f) : _friend(f) {}

    [[nodiscard]] control::SushiBuildInfo save_build_info() const
    {
        return _friend._save_build_info();
    }

    [[nodiscard]] control::MidiState save_midi_state() const
    {
        return _friend._save_midi_state();
    }

    [[nodiscard]] control::EngineState save_engine_state() const
    {
        return _friend._save_engine_state();
    }

    [[nodiscard]] std::vector<control::TrackState> save_tracks() const
    {
        return _friend._save_tracks();
    }

    void clear_all_tracks()
    {
        _friend._clear_all_tracks();
    }

private:
    SessionController& _friend;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_SESSION_CONTROLLER_H
