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
    MIND_DECLARE_NON_COPYABLE(Sample);
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
    float at(float position) const;

private:
    const float* _data{nullptr};
    int  _length{0};
};

/**
 * @brief Too avoid divisions by zero and extensive branching, we limit the
 * attack, decay and sustain times to some extremely short value and not 0.
 */
constexpr float SHORTEST_ENVELOPE_TIME = 0.00001f;


enum class EnvelopeState
{
    OFF,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
};

/**
 * @brief A basic, linear slope, ADSR envelope class.
 */
class Envelope
{
    MIND_DECLARE_NON_COPYABLE(Envelope);
public:
    Envelope() {};

    /**
     * @brief Set the envelope parameters.
     * @param attack Attack time in seconds.
     * @param decay Decay time in seconds.
     * @param sustain Sustain level, 0 - 1.
     * @param release Release time in seconds.
     */
    void set_parameters(float attack, float decay, float sustain, float release);

    /**
     * @brief Set the current samplerate
     * @param samplerate The samplerate in samples/second.
     */
    void set_samplerate(float samplerate) {_samplerate = samplerate;}

    /**
     * @brief Advance the envelope a given number of samples. and return
     *        its current value.
     * @param samples The number of samples to 'tick' the envelope.
     * @return The current envelope level.
     */
    float tick(int samples = 0);

    /**
     * @brief Get the envelopes current level without advancing it.
     * @return The current envelope level.
     */
    float level() const {return _current_level;}

    /**
     * @brief Analogous to the gate signal on an analog envelope. Setting gate
     *        to true will start the envelope in the attack phase and setting
     *        it to false will start the release phase.
     * @param gate Set to true for note on and false at note off.
     */
    void gate(bool gate);

    /**
     * @brief Returns true if the envelope is off, ie. the release phase is finished.
     * @return
     */
    bool is_off() {return _state == EnvelopeState::OFF;}

    /**
     * @brief Resets the envelope to 0 immediately. Bypassing any long release
     *        phase.
     */
    void reset();

private:
    float _attack{0};
    float _decay{0};
    float _sustain{1};
    float _release{0.1};
    float _current_level{0};
    float _release_level{0};
    float _samplerate{44100};

    EnvelopeState _state{EnvelopeState::OFF};
};


class Voice
{
    MIND_DECLARE_NON_COPYABLE(Voice);
public:
    Voice() {};

    Voice(int samplerate, Sample* sample) : _samplerate(samplerate), _sample(sample) {}

    /**
     * @brief Runtime samplerate configuration.
     * @param samplerate The playback samplerate.
     */
    void set_samplerate(int samplerate)
    {
        _samplerate = samplerate;
        _envelope.set_samplerate(samplerate);
    }

    /**
     * @brief Runtime sample configuration
     * @param sample
     */
    void set_sample(Sample* sample) {_sample = sample;}

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

    int _samplerate{44100};
    Sample* _sample;
    SamplePlayMode _state{SamplePlayMode::STOPPED};
    Envelope _envelope;
    int _current_note;
    float _playback_speed;
    float _velocity;
    float _playback_pos{0};
    int _start_offset{0};
    int _stop_offset{0};
};

} // end namespace sample_player_voice
#endif //SUSHI_SAMPLE_VOICE_H
