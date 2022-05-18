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
#include <algorithm>
#include <mutex>

#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
#include "ableton/Link.hpp"
#else //SUSHI_BUILD_WITH_ABLETON_LINK
#include "link_dummy.h"
#endif //SUSHI_BUILD_WITH_ABLETON_LINK

#include "twine/twine.h"

#include "library/rt_event.h"
#include "library/constants.h"
#include "transport.h"
#include "logging.h"

namespace sushi {
namespace engine {

constexpr float MIN_TEMPO = 20.0;
constexpr float MAX_TEMPO = 1000.0;
constexpr float PPQN_FLOAT = SUSHI_PPQN_TICK;


#if SUSHI_BUILD_WITH_ABLETON_LINK

/**
 * @brief Custom realtime clock for Link
 *        It is necessary to compile Link with another Clock implementation than the standard one
 *        as calling clock_get_time() is not safe to do from a Xenomai thread. Instead we supply
 *        our own clock implementation based on twine, which provides a threadsafe implementation
 *        for calling from both xenomai and posix contexts.
 */
class RtSafeClock
{
public:
    [[nodiscard]] std::chrono::microseconds micros() const
    {
        auto time = twine::current_rt_time();
        return std::chrono::microseconds(std::chrono::duration_cast<std::chrono::microseconds>(time));
    }
};

/**
 * @brief Wrapping Link with a custom clock
 */
class SushiLink : public ::ableton::BasicLink<RtSafeClock>
{
public:
  using Clock = RtSafeClock;

  explicit SushiLink(double bpm) : ::ableton::BasicLink<Clock>(bpm) { }
};

#endif

SUSHI_GET_LOGGER_WITH_MODULE_NAME("transport");

void peer_callback([[maybe_unused]] size_t peers)
{
    SUSHI_LOG_INFO("Ableton link reports {} peers connected ", peers);
}

void tempo_callback([[maybe_unused]] double tempo)
{
    SUSHI_LOG_DEBUG("Ableton link reports tempo is now {} bpm ", tempo);
}

void start_stop_callback([[maybe_unused]] bool playing)
{
    SUSHI_LOG_INFO("Ableton link reports {}", playing? "now playing" : "now stopped");
}

inline bool valid_time_signature(const TimeSignature& sig)
{
    return sig.numerator > 0 && sig.denominator > 0;
}

Transport::Transport(float sample_rate,
                     RtEventPipe* rt_event_pipe) : _samplerate(sample_rate),
                                                   _rt_event_dispatcher(rt_event_pipe),
                                                   _link_controller(std::make_unique<SushiLink>(DEFAULT_TEMPO))
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
    _state_change = PlayStateChange::UNCHANGED;

    _update_internals();

    switch (_syncmode)
    {
        case SyncMode::MIDI:       // Midi and Gate not implemented, so treat like internal
        case SyncMode::GATE_INPUT:
        case SyncMode::INTERNAL:
        {
            _update_internal_sync(samples - prev_samples);
            break;
        }

        case SyncMode::ABLETON_LINK:
        {
            _update_link_sync(_time);
            break;
        }
    }
    if (_playmode != PlayingMode::STOPPED)
    {
        _output_ppqn_ticks();
    }
}

void Transport::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::TEMPO:
            _set_tempo = std::clamp(event.tempo_event()->tempo(), MIN_TEMPO, MAX_TEMPO);
            break;

        case RtEventType::TIME_SIGNATURE:
        {
            auto time_signature = event.time_signature_event()->time_signature();
            if (time_signature != _time_signature && valid_time_signature(time_signature))
            {
                _time_signature = time_signature;
                _rt_event_dispatcher->send_event(RtEvent::make_time_signature_event(0, _time_signature));
            }
            break;
        }

        case RtEventType::PLAYING_MODE:
            _set_playmode = event.playing_mode_event()->mode();
            break;

        case RtEventType::SYNC_MODE:
        {
            auto mode = event.sync_mode_event()->mode();
#ifndef SUSHI_BUILD_WITH_ABLETON_LINK
            if (mode == SyncMode::ABLETON_LINK)
            {
                mode = SyncMode::INTERNAL;
            }
#endif
            if (mode != _syncmode)
            {
                _syncmode = mode;
                _rt_event_dispatcher->send_event(RtEvent::make_sync_mode_event(0, _syncmode));
            }
            break;
        }

        default:
            break;
    }
}

void Transport::set_time_signature(TimeSignature signature, bool update_via_event)
{
    if (valid_time_signature(signature))
    {
        if (update_via_event == false)
        {
            _time_signature = signature;
        }
        if (_link_controller->isEnabled())
        {
            _set_link_quantum(signature);
        }
    }
}

void Transport::set_tempo(float tempo, bool update_via_event)
{
    tempo = std::clamp(tempo, MIN_TEMPO, MAX_TEMPO);
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
    bool update = this->playing() != playing;
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
     * the same way most DAWs do it. This makes 3/4 and 6/8 behave identically, and
     * they will play beatsynched with 4/4, i.e. not on triplets. */
    _beats_per_bar = 4.0f * static_cast<float>(_time_signature.numerator) /
                            static_cast<float>(_time_signature.denominator);
}

void Transport::_update_internal_sync(int64_t samples)
{
    // We cannot assume chunk size is an absolute multiple of samples for all buffer sizes.
    auto chunks_passed = static_cast<double>(samples) / AUDIO_CHUNK_SIZE;

    if (_playmode != _set_playmode)
    {
        _state_change = _set_playmode == PlayingMode::STOPPED? PlayStateChange::STOPPING : PlayStateChange::STARTING;
        _playmode = _set_playmode;
        // Notify new playing mode
        _rt_event_dispatcher->send_event(RtEvent::make_playing_mode_event(0, _set_playmode));
    }

    double beats_per_chunk =  _set_tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
    _beats_per_chunk = beats_per_chunk;

    if (_state_change == PlayStateChange::STARTING) // Reset bar beat count when starting
    {
        _current_bar_beat_count = 0.0;
        _beat_count = 0.0;
        _bar_start_beat_count = 0.0;
    }
    else if (_playmode != PlayingMode::STOPPED)
    {
        _current_bar_beat_count += chunks_passed * beats_per_chunk;
        if (_current_bar_beat_count > _beats_per_bar)
        {
            _current_bar_beat_count = std::fmod(_current_bar_beat_count, _beats_per_bar);
            _bar_start_beat_count += _beats_per_bar;
        }
        _beat_count += chunks_passed * beats_per_chunk;
    }

    if (_tempo != _set_tempo)
    {
        // Notify tempo change
        _rt_event_dispatcher->send_event(RtEvent::make_tempo_event(0, _set_tempo));
        _tempo = _set_tempo;
    }
}

void Transport::_update_link_sync(Time timestamp)
{
    auto session = _link_controller->captureAudioSessionState();
    auto tempo = static_cast<float>(session.tempo());
    if (tempo != _set_tempo)
    {
        _set_tempo = tempo;
        // Notify new tempo
        _rt_event_dispatcher->send_event(RtEvent::make_tempo_event(0, tempo));
    }
    _tempo = tempo;

    if (session.isPlaying() != this->playing())
    {
        auto new_playmode = session.isPlaying() ? PlayingMode::PLAYING: PlayingMode::STOPPED;
        _state_change = new_playmode == PlayingMode::STOPPED? PlayStateChange::STOPPING : PlayStateChange::STARTING;
        _playmode = new_playmode;
        _set_playmode = new_playmode;
        // Notify new playing mode
        _rt_event_dispatcher->send_event(RtEvent::make_playing_mode_event(0, _set_playmode));
    }

    _beats_per_chunk =  _tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
    if (session.isPlaying())
    {
        _beat_count = session.beatAtTime(timestamp, _beats_per_bar);
        _current_bar_beat_count = session.phaseAtTime(timestamp, _beats_per_bar);
        _bar_start_beat_count = _beat_count - _current_bar_beat_count;
    }

    /* Due to the nature of the Xenomai RT architecture we cannot commit changes to
     * the session here as that would cause a mode switch. Instead, all changes need
     * to be made from the non rt thread */
}

void Transport::_output_ppqn_ticks()
{
    double beat = current_beats();
    if (current_state_change() == PlayStateChange::STARTING)
    {
        _last_tick_sent = beat;
    }

    double last_beat_this_chunk = current_beats(AUDIO_CHUNK_SIZE);
    double beat_period = last_beat_this_chunk - beat;
    auto tick_this_chunk = std::min(PPQN_FLOAT * (last_beat_this_chunk - _last_tick_sent), 2.0);

    while (tick_this_chunk >= 1.0)
    {
        double next_note_beat = _last_tick_sent + 1.0 / PPQN_FLOAT;
        auto fraction = next_note_beat - beat - std::floor(next_note_beat - beat);
        _last_tick_sent = next_note_beat;
        int offset = 0;
        if (fraction > 0)
        {
            /* If fraction is not positive, then there was a missed beat in an underrun */
            offset = std::min(static_cast<int>(std::round(AUDIO_CHUNK_SIZE * fraction / beat_period)),
                              AUDIO_CHUNK_SIZE - 1);
        }

        RtEvent tick = RtEvent::make_timing_tick_event(offset, 0);
        _rt_event_dispatcher->send_event(tick);
        tick_this_chunk = PPQN_FLOAT * (last_beat_this_chunk - _last_tick_sent);
    }
}

#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
void Transport::_set_link_playing(bool playing)
{
    auto session = _link_controller->captureAppSessionState();
    session.setIsPlaying(playing, _time);
    if (playing)
    {
        session.requestBeatAtTime(_beat_count, _time, _beats_per_bar);
        _link_controller->commitAppSessionState(session);
    }
}

void Transport::_set_link_tempo(float tempo)
{
    auto state = _link_controller->captureAppSessionState();
    state.setTempo(tempo, this->current_process_time());
    _link_controller->commitAudioSessionState(state);
}

void Transport::_set_link_quantum(TimeSignature signature)
{
    auto session = _link_controller->captureAppSessionState();
    if (session.isPlaying())
    {
        // TODO - Consider what to do for non-integer quanta. Multiply with gcd until we get an integer ratio?
        int quantum = std::max(1, (4 * signature.denominator) / signature.numerator);
        session.requestBeatAtTime(_beat_count, _time, quantum);
        _link_controller->commitAppSessionState(session);
    }
}

#else
void Transport::_set_link_playing(bool /*playing*/) {}
void Transport::_set_link_tempo(float /*tempo*/) {}
void Transport::_set_link_quantum(TimeSignature /*signature*/) {}
#endif




} // namespace engine
} // namespace sushi