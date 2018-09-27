#include <cassert>
#include <cmath>

#ifdef SUSHI_BUILD_WITH_ABLETON_LINK
#include "ableton/Link.hpp"
#else
#include "link_dummy.h"
#endif

#include <library/rt_event.h>
#include "library/constants.h"
#include "transport.h"
#include "logging.h"

namespace sushi {
namespace engine {

MIND_GET_LOGGER;

void peer_callback(size_t peers)
{
    MIND_LOG_INFO("Ableton link reports {} peers connected ", peers);
}

void tempo_callback(double tempo)
{
    MIND_LOG_INFO("Ableton link reports tempo is now {} bpm ", tempo);
}

void start_stop_callback(bool playing)
{
    MIND_LOG_INFO("Ableton link reports {}", playing? "now playing" : "now stopped");
}


Transport::Transport(float sample_rate) : _samplerate(sample_rate)
{
    _link_controller.reset(new ableton::Link(sample_rate));
    _link_controller->setNumPeersCallback(peer_callback);
    _link_controller->setTempoCallback(tempo_callback);
    _link_controller->setStartStopCallback(start_stop_callback);
    _link_controller->enableStartStopSync(true);
    _link_controller->enable(true);
}

Transport::~Transport()
{

}

void Transport::set_time(Time timestamp, int64_t samples)
{
    _time = timestamp + _latency;
    int64_t prev_samples = _sample_count;
    _sample_count = samples;

    _update_internals();

#ifndef SUSHI_BUILD_WITH_ABLETON_LINK
    if (_sync_mode == SyncMode::ABLETON_LINK) _sync_mode = SyncMode::INTERNAL;
#endif

    switch (_sync_mode)
    {
        case SyncMode::MIDI_SLAVE: // Not implemented, so treat like master
        case SyncMode::INTERNAL:
            _update_internal_sync(samples - prev_samples);
            break;

        case SyncMode::ABLETON_LINK:
        {
            //if (_new_playmode || _new_tempo || ++_link_update_count < LINK_UPDATE_RATE)
            //{
                _update_link_sync(_time);
                _link_update_count = 0;
            //}
        }
    }
    _new_tempo = false;
    _new_playmode = false;
    //static int logg = 0;
    //logg = ++logg % 1000;
    //MIND_LOG_INFO_IF(logg== 0, "Current beats: {}, bar: {}, bar start: {}", _beat_count, _current_bar_beat_count, _bar_start_beat_count);
}

void Transport::set_time_signature(TimeSignature signature)
{
    assert(signature.numerator > 0);
    assert(signature.denominator > 0);
    _time_signature = signature;
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
    _beats_per_chunk =  _set_tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
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
    if (chunks_passed < 1)
    {
        //chunks_passed = 1;
        MIND_LOG_INFO("First time {}", chunks_passed);
    }

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
    bool currently_playing = this->playing();
    if (_new_playmode)
    {
        bool playing = _set_playmode == PlayingMode::PLAYING || _set_playmode == PlayingMode::RECORDING;
        session.setIsPlaying(playing, timestamp);
    }
    if (!currently_playing && session.isPlaying()) // Start playing
    {
        /* Request beat at the start of next bar */
        session.requestBeatAtStartPlayingTime(_bar_start_beat_count + _beats_per_bar, _beats_per_bar);
        _playmode = (_set_playmode == PlayingMode::STOPPED)? PlayingMode::PLAYING : _set_playmode;
    }
    if (currently_playing && !session.isPlaying()) // Link stopped playing
    {
        _playmode = PlayingMode::STOPPED;
    }
    if (_new_tempo)
    {
        session.setTempo(_set_tempo, timestamp);
    }
    _tempo = session.tempo();
    _link_controller->commitAudioSessionState(session);
    //if (this->playing())
    //{
        _beat_count = session.beatAtTime(timestamp, _beats_per_bar);
        _current_bar_beat_count = session.phaseAtTime(timestamp, _beats_per_bar);
        _bar_start_beat_count = _beat_count - _current_bar_beat_count;
    //}
}


} // namespace engine
} // namespace sushi