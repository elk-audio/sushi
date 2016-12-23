#include <cassert>
#include <cmath>
#include <iostream>

#include "plugins/sample_player_voice.h"

namespace sample_player_voice {

inline float Sample::at(float position)
{
    assert(position >= 0);

    float pos_int = std::floor(position);
    float weight = position - pos_int;

    // lrint is claimed to be faster than a cast from float to integer
    int sample_pos = static_cast<int>(lrint(pos_int));
    float sample_low = (sample_pos < _length) ? _data[sample_pos] : 0.0f;
    float sample_high = (sample_pos + 1 < _length) ? _data[sample_pos + 1] : 0.0f;

    return (sample_high * weight + sample_low * (1 - weight));
}

/* Using a method inspired by a vst example to track notes. The voice
 * itself has no event queue so only one note one and one note off
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
    _playback_speed = powf(2, note - 60);
}

/* No release functionality handled atm, notes are simply cut off
 * when a note off is received. Release velocity is not handled
 * either. Has any synth ever supported it?
 */
void Voice::note_off(float /*velocity*/, int offset)
{
    assert(offset < AUDIO_CHUNK_SIZE);

    _state = SamplePlayMode::STOPPING;
    _stop_offset = offset;
}

void Voice::reset()
{
    _state = SamplePlayMode::STOPPED;
}

void Voice::render(sushi::SampleBuffer<AUDIO_CHUNK_SIZE>& output_buffer)
{
    if (_state == SamplePlayMode::STOPPED)
    {
        return;
    }
    /* Handle only mono samples for now */
    float* out = output_buffer.channel(0);
    for (int i = _start_offset; i < _stop_offset; i++)
    {
        out[i] += _sample->at(_playback_pos) * _velocity;
        _playback_pos += _playback_speed;
    }

    /* Handle state changes and reset render limits */
    switch (_state)
    {
        case SamplePlayMode::STARTING:
            _state = SamplePlayMode::PLAYING;
            _start_offset = 0;
            break;

        case SamplePlayMode::STOPPING:
            _state = SamplePlayMode::STOPPED;
            break;

        default:
            break;
    }

}

}// namespace sample_player_voice