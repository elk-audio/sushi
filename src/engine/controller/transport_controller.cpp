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
 * @brief Implementation of external control interface of sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "transport_controller.h"

#include "elklog/static_logger.h"

#include "controller_common.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi::internal::engine::controller_impl {

TransportController::TransportController(engine::BaseEngine* engine) : _engine(engine),
                                                                       _transport(engine->transport()),
                                                                       _event_dispatcher(engine->event_dispatcher())
{}

float TransportController::get_samplerate() const
{
    ELKLOG_LOG_DEBUG("get_samplerate called");
    return _engine->sample_rate();
}

control::PlayingMode TransportController::get_playing_mode() const
{
    ELKLOG_LOG_DEBUG("get_playing_mode called");
    return to_external(_transport->playing_mode());
}

void TransportController::set_playing_mode(control::PlayingMode playing_mode)
{
    ELKLOG_LOG_DEBUG("set_playing_mode called");
    auto event = new SetEnginePlayingModeStateEvent(to_internal(playing_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

control::SyncMode TransportController::get_sync_mode() const
{
    ELKLOG_LOG_DEBUG("get_sync_mode called");
    return to_external(_transport->sync_mode());
}

void TransportController::set_sync_mode(control::SyncMode sync_mode)
{
    ELKLOG_LOG_DEBUG("set_sync_mode called");
    auto event = new SetEngineSyncModeEvent(to_internal(sync_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

float TransportController::get_tempo() const
{
    ELKLOG_LOG_DEBUG("get_tempo called");
    return _transport->current_tempo();
}

control::ControlStatus TransportController::set_tempo(float tempo)
{
    ELKLOG_LOG_DEBUG("set_tempo called with tempo {}", tempo);
    auto event = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return control::ControlStatus::OK;
}

control::TimeSignature TransportController::get_time_signature() const
{
    ELKLOG_LOG_DEBUG("get_time_signature called");
    return to_external(_transport->time_signature());
}

control::ControlStatus TransportController::set_time_signature(control::TimeSignature signature)
{
    ELKLOG_LOG_DEBUG("set_time_signature called with signature {}/{}", signature.numerator, signature.denominator);
    auto event = new SetEngineTimeSignatureEvent(to_internal(signature), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return control::ControlStatus::OK;
}

} // end namespace sushi::internal::engine::controller_impl