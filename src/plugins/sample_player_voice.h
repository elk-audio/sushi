/**
 * @brief Voice class for sample player plugin
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SAMPLE_VOICE_H
#define SUSHI_SAMPLE_VOICE_H

#include "library/sample_buffer.h"

namespace sample_player_voice {

enum class SamplePlayMode
{
    STOPPED,
    STARTING,
    PLAYING,
    STOPPING
};


/**
 * @brief Class to wrap an audio sample into a prettier interface
 */
class Sample
{
public:
    Sample() : _data(nullptr), _length(0) {}

    Sample(const float* sample, int length) : _data(sample), _length(length) {}

    /**
     * @brief Set the sample to wrap.
     * @param sample Pointer to sample array, Sample does not take ownership of the data.
     * @param length Number of samples in the data.
     */
    void set_sample(const float* sample, int length)
    {
        _data = sample;
        _length = length;
    }

    /**
     * @brief Return the value at sample position. Does linear interpolation
     * @param position The position in the sample buffer.
     * @return A linearily interpolated sample value.
     */
    float at(float position);

private:
    const float* _data{nullptr};
    int  _length{0};
};


class Voice
{
public:
    Voice() {};

    Voice(int samplerate, Sample* sample) : _samplerate(samplerate), _sample(sample) {}

    /**
     * @brief Runtime samplerate configuration.
     * @param samplerate The playback samplerate.
     */
    void set_samplerate(int samplerate) {_samplerate = samplerate;}

    /**
     * @brief Runtime sample configuration
     * @param sample
     */
    void set_sample(Sample* sample) {_sample = sample;}

    /**
     * @brief Is currently playing sound.
     * @return True if currently playing sound.
     */
    bool active() {return (_state != SamplePlayMode::STOPPED);}

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

    int _samplerate{44100};
    Sample* _sample;
    SamplePlayMode _state{SamplePlayMode::STOPPED};
    int _current_note;
    float _playback_speed;
    float _velocity;
    float _playback_pos{0};
    int _start_offset{0};
    int _stop_offset{0};
};

} // end namespace sample_player_voice
#endif //SUSHI_SAMPLE_VOICE_H
