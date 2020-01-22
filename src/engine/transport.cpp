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
 * @brief Main entry point to Sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>
#include <cmath>

#include "link_include.h"

#include <library/rt_event.h>
#include "library/constants.h"
#include "transport.h"
#include "logging.h"

namespace sushi {
namespace engine {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("transport");

void peer_callback(size_t peers)
{
    SUSHI_LOG_INFO("Ableton link reports {} peers connected ", peers);
}

void tempo_callback(double tempo)
{
    SUSHI_LOG_INFO("Ableton link reports tempo is now {} bpm ", tempo);
}

void start_stop_callback(bool playing)
{
    SUSHI_LOG_INFO("Ableton link reports {}", playing? "now playing" : "now stopped");
}


Transport::Transport(float sample_rate) : _samplerate(sample_rate),
                                           _link_controller(std::make_unique<ableton::Link>(DEFAULT_TEMPO))
{
    _link_controller->setNumPeersCallback(peer_callback);
    _link_controller->setTempoCallback(tempo_callback);
    _link_controller->setStartStopCallback(start_stop_callback);
    _link_controller->enableStartStopSync(true);
}

Transport::~Transport() = default;

void Transport::set_time(Time timestamp, int64_t samples)
{
    _time = timestamp + _latency;
    int64_t prev_samples = _sample_count;
    _sample_count = samples;

    _update_internals();

    switch (_syncmode)
    {
        case SyncMode::MIDI: // Not implemented, so treat like master
        case SyncMode::GATE_INPUT:
        case SyncMode::INTERNAL:
            _update_internal_sync(samples - prev_samples);
            break;

        case SyncMode::ABLETON_LINK:
        {
            _update_link_sync(_time);
            break;
        }
    }

    //static int logg = 0;
    //logg = ++logg % 1000;
    //SUSHI_LOG_INFO_IF(logg== 0, "Current beats: {}, bar: {}, bar start: {}", _beat_count, _current_bar_beat_count, _bar_start_beat_count);
}

void Transport::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::TEMPO:
            _set_tempo = event.tempo_event()->tempo();
            break;

        case RtEventType::TIME_SIGNATURE:
            _time_signature = event.time_signature_event()->time_signature();
            break;

        case RtEventType::PLAYING_MODE:
            _set_playmode = event.playing_mode_event()->mode();
            break;

        case RtEventType::SYNC_MODE:
            _syncmode = event.sync_mode_event()->mode();
            break;

        default:
            break;
    }
}

void Transport::set_time_signature(TimeSignature signature, bool update_via_event)
{
    assert(signature.numerator > 0);
    assert(signature.denominator > 0);
    if (update_via_event == false)
    {
        _time_signature = signature;
    }
}

void Transport::set_tempo(float tempo, bool update_via_event)
{
    if (update_via_event == false)
    {
        _set_tempo = tempo;
        _tempo = tempo;
    }
    _set_link_tempo(tempo);
}

void Transport::set_playing_mode(PlayingMode mode, bool update_via_event)
{
    bool playing = mode != PlayingMode::STOPPED;
    bool update = this->playing() == playing;
    if (update)
    {
        if (_link_controller->isEnabled())
        {
            _set_link_playing(playing);
        }
    }

    if (update_via_event == false)
    {
        _set_playmode = mode;
    }
}

void Transport::set_sync_mode(SyncMode mode, bool update_via_event)
{
#ifndef SUSHI_BUILD_WITH_ABLETON_LINK
    if (mode == SyncMode::ABLETON_LINK)
    {
        SUSHI_LOG_INFO("Ableton Link sync mode requested, but sushi was built without Link support");
        return;
    }
#endif
    switch (mode)
    {
        case SyncMode::INTERNAL:
        case SyncMode::MIDI:
        case SyncMode::GATE_INPUT:
            _link_controller->enable(false);
            break;

        case SyncMode::ABLETON_LINK:
            _link_controller->enable(true);
            _set_link_playing(_set_playmode != PlayingMode::STOPPED);
            break;
    }
    if (update_via_event == false)
    {
        _syncmode = mode;
    }
}

double Transport::current_bar_beats(int samples) const
{
    if (_playmode != PlayingMode::STOPPED)
    {
        double offset = _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
        return std::fmod(_current_bar_beat_count + offset, _beats_per_bar);
    }
    return _current_bar_beat_count;
}

double Transport::current_beats(int samples) const
{
    if (_playmode != PlayingMode::STOPPED)
    {
        return _beat_count + _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
    }
    return _beat_count;
}

void Transport::_update_internals()
{
    assert(_samplerate > 0.0f);
    /* Time signatures are seen in relation to 4/4 and remapped to quarter notes
     * the same way most DAWs do it. This makes 3/4 and 6/8 behave identically and
     * they will play beatsynched with 4/4, i.e. not on triplets. */
    _beats_per_bar = 4.0f * static_cast<float>(_time_signature.numerator) /
                            static_cast<float>(_time_signature.denominator);
}

void Transport::_update_internal_sync(int64_t samples)
{
    /* Assume that if there are missed callbacks, the numbers of samples
     * will still be a multiple of AUDIO_CHUNK_SIZE */
    auto chunks_passed = samples / AUDIO_CHUNK_SIZE;
    /*if (chunks_passed < 1)
    {
        //chunks_passed = 1;
        SUSHI_LOG_INFO("First time {}", chunks_passed);
    }*/

    _beats_per_chunk =  _set_tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
    if (_playmode != PlayingMode::STOPPED)
    {
        _current_bar_beat_count += chunks_passed * _beats_per_chunk;
        if (_current_bar_beat_count > _beats_per_bar)
        {
            _current_bar_beat_count = std::fmod(_current_bar_beat_count, _beats_per_bar);
            _bar_start_beat_count += _beats_per_bar;
            //_bar_start_beat_count += _beats_per_bar * std::floor(_current_bar_beat_count / _beats_per_bar);

        }
        _beat_count += chunks_passed * _beats_per_chunk;
    }
    _tempo = _set_tempo;
    _playmode = _set_playmode;
}

void Transport::_update_link_sync(Time timestamp)
{
    auto session = _link_controller->captureAudioSessionState();
    _tempo = static_cast<float>(session.isPlaying());
    _playmode = session.isPlaying() ? (_set_playmode != PlayingMode::STOPPED?
            _set_playmode : PlayingMode::PLAYING) : PlayingMode::STOPPED;

    _beats_per_chunk =  _tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;

    if (session.isPlaying())
    {
        _beat_count = session.beatAtTime(timestamp, _beats_per_bar);
        _current_bar_beat_count = session.phaseAtTime(timestamp, _beats_per_bar);
        _bar_start_beat_count = _beat_count - _current_bar_beat_count;
    }
    /* Due to the nature of the Xenomai RT architecture we cannot commit changes to
     * the session here as that would cause a mode switch. Instead all changes need
     * to be made from the non rt thread */
}

void Transport::_set_link_playing(bool playing)
{
#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
    auto state = _link_controller->captureAppSessionState();
    state.setIsPlaying(playing, _time);
    if (playing)
    {
        /* Request beat at the start of next bar for synchronised launch*/
        state.requestBeatAtStartPlayingTime(_bar_start_beat_count + _beats_per_bar, _beats_per_bar);
    }
    _link_controller->commitAppSessionState(state);
#endif
}

void Transport::_set_link_tempo(float tempo)
{
#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
    auto state = _link_controller->captureAppSessionState();
    state.setTempo(tempo, this->current_process_time());
    _link_controller->commitAudioSessionState(state);
#endif
}

} // namespace engine
} // namespace sushi