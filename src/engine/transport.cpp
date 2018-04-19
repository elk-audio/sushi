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
    /*_link_controller.reset(new ableton::Link(sample_rate));
    _link_controller->setNumPeersCallback(peer_callback);
    _link_controller->setTempoCallback(tempo_callback);
    _link_controller->setStartStopCallback(start_stop_callback);
    _link_controller->enableStartStopSync(true);
    _link_controller->enable(true);*/
}

Transport::~Transport()
{

}

void Transport::set_time(Time timestamp, int64_t samples)
{
    _time = timestamp + _latency;
    int64_t prev_samples = _sample_count;
    _update_internals();

    /* Assume that if there is a missed callback, the numbers of samples
     * will still be an even multiple of AUDIO_CHUNK_SIZE */
    auto chunks_passed = (samples - prev_samples) / AUDIO_CHUNK_SIZE;
    _sample_count = samples;

    _current_bar_beat_count +=  chunks_passed * _beats_per_chunk;
    if (_current_bar_beat_count > _beats_per_bar)
    {
        _current_bar_beat_count = std::fmod(_current_bar_beat_count, _beats_per_bar);
        _bar_start_beat_count += _beats_per_bar;
    }
    _beat_count += chunks_passed * _beats_per_chunk;

    switch (_sync_mode)
    {
        case SyncMode::MIDI_SLAVE: // Not implemented, so treat like master
        case SyncMode::INTERNAL:
            //_tempo = _set_tempo;
            break;

        case SyncMode::ABLETON_LINK:
        {
            if (++_link_update_count < LINK_UPDATE_RATE)
            {
                //_handle_link_sync();
                _link_update_count = 0;
            }
        }
    }
}

void Transport::set_time_signature(TimeSignature signature)
{
    assert(signature.numerator > 0);
    assert(signature.denominator > 0);
    _time_signature = signature;
}

double Transport::current_bar_beats(int samples) const
{
    double offset = _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
    return std::fmod(_current_bar_beat_count + offset, _beats_per_bar);
}

double Transport::current_beats(int samples) const
{
    return _beat_count + _beats_per_chunk * static_cast<double>(samples) / AUDIO_CHUNK_SIZE;
}

void Transport::_update_internals()
{
    assert(_samplerate > 0.0f);
    _beats_per_chunk =  _tempo / 60.0 * static_cast<double>(AUDIO_CHUNK_SIZE) / _samplerate;
    /* Time signatures are seen in relation to 4/4 and remapped to quarter notes
     * the same way most DAWs seem to do it. This makes 3/4 and 6/8 identical and
     * they will play beatsynched with 4/4, i.e. not on triplets. */
    _beats_per_bar = 4.0f * static_cast<float>(_time_signature.numerator) /
                            static_cast<float>(_time_signature.denominator);
}

/*void Transport::_handle_link_sync()
{
    auto session = _link_controller.captureAudioSessionState();
    _tempo = session.tempo();

}*/

} // namespace engine
} // namespace sushi