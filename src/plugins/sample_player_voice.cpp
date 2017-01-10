#include <cassert>
#include <cmath>
#include <iostream>

#include "plugins/sample_player_voice.h"

namespace sample_player_voice {

inline float Sample::at(float position) const
{
    assert(position >= 0);
    assert(_data);

    int sample_pos = static_cast<int>(position);
    float weight = position - std::floor(position);
    float sample_low = (sample_pos < _length) ? _data[sample_pos] : 0.0f;
    float sample_high = (sample_pos + 1 < _length) ? _data[sample_pos + 1] : 0.0f;

    return (sample_high * weight + sample_low * (1.0f - weight));
}


void Envelope::set_parameters(float attack, float decay, float sustain, float release)
{
    _attack  = (attack > SHORTEST_ENVELOPE_TIME ? attack : SHORTEST_ENVELOPE_TIME);
    _decay   = (decay > SHORTEST_ENVELOPE_TIME ? decay : SHORTEST_ENVELOPE_TIME);
    _sustain = sustain;
    _release = (release > SHORTEST_ENVELOPE_TIME ? release : SHORTEST_ENVELOPE_TIME);
}


inline float Envelope::tick(int samples)
{
    /* The _samplerate * time factors could be precomputed when updating
     * parameters in order to save some cpu time in the realtime loop. */
    switch (_state)
    {
        case EnvelopeState::OFF:
            break;

        case EnvelopeState::ATTACK:
            _current_level += samples / (_samplerate * _attack );
            if (_current_level >= 1)
            {
                _state = EnvelopeState::DECAY;
                _current_level = 1.0f;
            }
            break;

        case EnvelopeState::DECAY:
            _current_level -= (1.0f - _sustain) * samples / (_samplerate * _decay);
            if (_current_level <= _sustain)
            {
                _state = EnvelopeState::SUSTAIN;
                _current_level = _sustain;
            }
            break;

        case EnvelopeState::SUSTAIN:
            /* fixed level, wait for a gate release/note off */
            break;

        case EnvelopeState::RELEASE:
            _current_level -= (1.0f - _release_level) * samples / (_samplerate * _release);
            if (_current_level < 0.0f)
            {
                _state = EnvelopeState::OFF;
                _current_level = 0.0f;
            }
            break;
    }
    return _current_level;
}

void Envelope::gate(bool gate)
{
    if (gate) /* If the envelope is running, it's simply restarted here */
    {
        _state = EnvelopeState::ATTACK;
        _current_level = 0.0f;
    }
    else /* Gate off - go to release phase */
    {
        _state = EnvelopeState::RELEASE;
        _release_level = _current_level;
    }
}

void Envelope::reset()
{
    _state = EnvelopeState::OFF;
    _current_level = 0.0f;
}




/* Using a method inspired by a vst example to track notes. The voice
 * itself has no event queue so only one note on and one note off
 * event can be triggered per audio chunk. This puts a limit on the
 * number of notes per second an instrument can play, which is the
 * number of audio chunks per second * the polyphony.
 * For 6 voices and 64 sample chunks this amounts to a maximum of 4000
 * notes per second. Which should be still be enough for all but the
 * most absurd Black Midi compositions.
 */
void Voice::note_on(int note, float velocity, int offset)
{
    assert(offset < AUDIO_CHUNK_SIZE);

    /* Completely ignore any currently playing note, it will be cut off abruptly */
    _state = SamplePlayMode::STARTING;
    _velocity = velocity;
    _start_offset = offset;
    _stop_offset = AUDIO_CHUNK_SIZE;
    _playback_pos = 0.0f;
    _current_note = note;
    _playback_speed = powf(2, (note - 60)/12.0f);
    _envelope.gate(true);
}

/* Release velocity is ignored atm. Has any synth ever supported it? */
void Voice::note_off(float /*velocity*/, int offset)
{
    assert(offset < AUDIO_CHUNK_SIZE);

    _state = SamplePlayMode::STOPPING;
    _stop_offset = offset;
}

void Voice::reset()
{
    _state = SamplePlayMode::STOPPED;
    _envelope.reset();
}

void Voice::render(sushi::SampleBuffer<AUDIO_CHUNK_SIZE>& output_buffer)
{
    if (_state == SamplePlayMode::STOPPED)
    {
        return;
    }
    /* Handle only mono samples for now */
    float* out = output_buffer.channel(0);

    for (int i = _start_offset; i < _stop_offset; ++i)
    {
        out[i] += _sample->at(_playback_pos) * _velocity * _envelope.tick(1);
        _playback_pos += _playback_speed;
    }

    /* If there is a note off event, set the envelope to off and
     * render the rest of the chunk */
    if (_state == SamplePlayMode::STOPPING)
    {
        _envelope.gate(false);
        for (int i = _stop_offset + 1 ; i < AUDIO_CHUNK_SIZE; ++i)
        {
            out[i] += _sample->at(_playback_pos) * _velocity * _envelope.tick(1);
            _playback_pos += _playback_speed;
        }
    }

    /* Handle state changes and reset render limits */
    switch (_state)
    {
        case SamplePlayMode::STARTING:
            _state = SamplePlayMode::PLAYING;
            _start_offset = 0;
            break;

        case SamplePlayMode::STOPPING:
            if (_envelope.finished())
            {
                _state = SamplePlayMode::STOPPED;
            }
            break;

        default:
            break;
    }

}

}// namespace sample_player_voice