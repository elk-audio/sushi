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
 * @brief Voice class for sample player plugin
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SAMPLE_VOICE_H
#define SUSHI_SAMPLE_VOICE_H

#include "dsp_library/envelopes.h"
#include "dsp_library/sample_wrapper.h"
#include "sushi/sample_buffer.h"

namespace sushi {
namespace sample_player_voice {

// TODO eventually make this configurable
constexpr float SAMPLE_FILE_RATE = 44100.0f;

enum class SamplePlayMode
{
    STOPPED,
    STARTING,
    PLAYING,
    STOPPING
};

class Voice
{
    SUSHI_DECLARE_NON_COPYABLE(Voice);
public:
    Voice() = default;

    Voice(float samplerate, dsp::Sample* sample) : _samplerate(samplerate), _sample(sample) {}

    /**
     * @brief Runtime samplerate configuration.
     * @param samplerate The playback samplerate.
     */
    void set_samplerate(float samplerate)
    {
        _playback_speed = _playback_speed * samplerate / _samplerate;
        _envelope.set_samplerate(samplerate / AUDIO_CHUNK_SIZE);
        _samplerate = samplerate;
    }

    /**
     * @brief Runtime sample configuration
     * @param sample
     */
    void set_sample(dsp::Sample* sample) {_sample = sample;}

    /**
     * @brief Set the envelope parameters.
     */
    void set_envelope(float attack, float decay, float sustain, float release)
    {
        _envelope.set_parameters(attack, decay, sustain, release);
    }

    /**
     * @brief Is currently playing sound.
     * @return True if currently playing sound.
     */
    bool active() {return (_state != SamplePlayMode::STOPPED);}

    /**
     * @brief Is currently in the release phase but still playing.
     * @return True if note is currently off but still sounding.
     */
    bool stopping() {return _state == SamplePlayMode::STOPPING;}

    /**
     * @brief Return the current note being played, if any.
     * @return The current note as a midi note number.
     */
    int current_note() {return _current_note;}

    /**
     * @brief Play a new note within this audio chunk
     * @param note The midi note number to play, with 60 as middle C.
     * @param velocity Velocity of the note to play. 0 to 1.
     * @param time_offset Offset in samples from the start of the chunk.
     */
    void note_on(int note, float velocity, int offset);

    /**
     * @brief Stop the currently playing note.
     * @param velocity Release velocity, not used, only included for completeness.
     * @param time_offset Offset in samples from start of current audio chunk.
     */
    void note_off(float velocity, int offset);

    /**
     * @brief Reset the voice and kill all sound.
     */
    void reset();

    /**
     * @brief Render one chunk of audio.
     * @param output_buffer Target buffer.
     */
    void render(sushi::SampleBuffer<AUDIO_CHUNK_SIZE>& output_buffer);

private:
    float _samplerate{44100};
    dsp::Sample* _sample;
    SamplePlayMode _state{SamplePlayMode::STOPPED};
    dsp::AdsrEnvelope _envelope;
    int _current_note;
    float _playback_speed;
    float _velocity_gain;
    double _playback_pos{0.0};
    int _start_offset{0};
    int _stop_offset{0};
};

} // namespace sushi
} // namespace sample_player_voice
#endif //SUSHI_SAMPLE_VOICE_H
