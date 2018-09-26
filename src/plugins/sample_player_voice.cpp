#include <cassert>
#include <cmath>

#include "plugins/sample_player_voice.h"

namespace sample_player_voice {

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
    offset = std::min(offset, AUDIO_CHUNK_SIZE - 1);

    /* Completely ignore any currently playing note, it will be cut off abruptly */
    _state = SamplePlayMode::STARTING;
    /* Quadratic velocity curve */
    _velocity_gain = velocity * velocity;
    _start_offset = offset;
    _stop_offset = AUDIO_CHUNK_SIZE;
    _playback_pos = 0.0;
    _current_note = note;
    /* The root note of the sample is assumed to be C4 in 44100 Hz*/
    _playback_speed = powf(2, (note - 60)/12.0f) * _samplerate / SAMPLE_FILE_RATE;
    _envelope.gate(true);
}

/* Release velocity is ignored atm. Has any synth ever supported it? */
void Voice::note_off(float /*velocity*/, int offset)
{
    assert(offset < AUDIO_CHUNK_SIZE);
    if (_state == SamplePlayMode::PLAYING || _state == SamplePlayMode::STARTING)
    {
        _state = SamplePlayMode::STOPPING;
        _stop_offset = offset;
    }
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
        out[i] += _sample->at(_playback_pos) * _velocity_gain * _envelope.tick(1);
        _playback_pos += _playback_speed;
    }

    /* If there is a note off event, set the envelope to off and
     * render the rest of the chunk */
    if (_state == SamplePlayMode::STOPPING)
    {
        _envelope.gate(false);
        for (int i = _stop_offset; i < AUDIO_CHUNK_SIZE; ++i)
        {
            out[i] += _sample->at(_playback_pos) * _velocity_gain * _envelope.tick(1);
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