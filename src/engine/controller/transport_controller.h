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

#ifndef SUSHI_TRANSPORT_CONTROLLER_H
#define SUSHI_TRANSPORT_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi::internal::engine::controller_impl {

class TransportController : public control::TransportController
{
public:
    explicit TransportController(BaseEngine* engine);

    float get_samplerate() const override;

    control::PlayingMode get_playing_mode() const override;

    control::SyncMode get_sync_mode() const override;

    control::TimeSignature get_time_signature() const override;

    float get_tempo() const override;

    void set_sync_mode(control::SyncMode sync_mode) override;

    void set_playing_mode(control::PlayingMode playing_mode) override;

    control::ControlStatus set_tempo(float tempo) override;

    control::ControlStatus set_time_signature(control::TimeSignature signature) override;

private:
    BaseEngine*                         _engine;
    engine::Transport*                  _transport;
    dispatcher::BaseEventDispatcher*    _event_dispatcher;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_TRANSPORT_CONTROLLER_H
