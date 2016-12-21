/**
 * @brief Voice class for sample player plugin
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SAMPLE_VOICE_H
#define SUSHI_SAMPLE_VOICE_H

namespace sample_player_voice {



/**
 * @brief Class to wrap an audio sample into a prettier interface
 */
class Sample
{
public:
    Sample();

    Sample(const float* sample, int length) : _data(sample), _length(length) {}

    /**
     * @brief Set the sample to wrap.
     * @param sample Pointer to first sample, Sample does not take ownership of the data.
     * @param length Number of samples in the data.
     */
    void set_sample(const float* sample, int length)
    {
        _data = sample;
        _length = length;
    }

    /**
     * @brief Return the length of the sample
     * @return The length of the sample in samples.
     */
    int length()
    { return _length; }

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
    Voice();

private:
    int _samplerate;

};

} // end namespace sample_player_voice
#endif //SUSHI_SAMPLE_VOICE_H
