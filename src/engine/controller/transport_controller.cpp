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
 * @brief Implementation of external control interface of sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "transport_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

/* Convenience conversion functions between external and internal
 * enums and data structs */
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
        case SyncMode::MIDI:         return ext::SyncMode::MIDI;
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
        case ext::SyncMode::MIDI:     return sushi::SyncMode::MIDI;
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

TransportController::TransportController(sushi::engine::BaseEngine* engine) : _engine(engine),
                                                                              _transport(engine->transport()),
                                                                              _event_dispatcher(engine->event_dispatcher())
{}

float TransportController::get_samplerate() const
{
    SUSHI_LOG_DEBUG("get_samplerate called");
    return _engine->sample_rate();
}

ext::PlayingMode TransportController::get_playing_mode() const
{
    SUSHI_LOG_DEBUG("get_playing_mode called");
    return to_external(_transport->playing_mode());
}

void TransportController::set_playing_mode(ext::PlayingMode playing_mode)
{
    SUSHI_LOG_DEBUG("set_playing_mode called");
    auto event = new SetEnginePlayingModeStateEvent(to_internal(playing_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

ext::SyncMode TransportController::get_sync_mode() const
{
    SUSHI_LOG_DEBUG("get_sync_mode called");
    return to_external(_transport->sync_mode());
}

void TransportController::set_sync_mode(ext::SyncMode sync_mode)
{
    SUSHI_LOG_DEBUG("set_sync_mode called");
    auto event = new SetEngineSyncModeEvent(to_internal(sync_mode), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
}

float TransportController::get_tempo() const
{
    SUSHI_LOG_DEBUG("get_tempo called");
    return _transport->current_tempo();
}

ext::ControlStatus TransportController::set_tempo(float tempo)
{
    SUSHI_LOG_DEBUG("set_tempo called with tempo {}", tempo);
    auto event = new SetEngineTempoEvent(tempo, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::TimeSignature TransportController::get_time_signature() const
{
    SUSHI_LOG_DEBUG("get_time_signature called");
    return to_external(_transport->time_signature());
}

ext::ControlStatus TransportController::set_time_signature(ext::TimeSignature signature)
{
    SUSHI_LOG_DEBUG("set_time_signature called with signature {}/{}", signature.numerator, signature.denominator);
    auto event = new SetEngineTimeSignatureEvent(to_internal(signature), IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi