/**
 * @brief Wrapper for an audio sample to provide sample interpolation in an OO interface
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_AUDIO_SAMPLE_H
#define SUSHI_AUDIO_SAMPLE_H

namespace dsp {

/**
 * @brief Class to wrap a mono audio sample into a prettier interface
 */
class Sample
{
    MIND_DECLARE_NON_COPYABLE(Sample);
public:
    Sample() {}

    Sample(const float* sample, int length) : _data(sample), _length(length) {}

    /**
     * @brief Set the sample to wrap.
     * @param sample Pointer to sample array, Sample does not take ownership of the data.
     * @param length Number of samples in the data.
     */
    void set_sample(const float* sample_data, int length)
    {
        _data = sample_data;
        _length = length;
    }

    /**
     * @brief Return the value at sample position. Does linear interpolation
     * @param position The position in the sample buffer.
     * @return A linearily interpolated sample value.
     */
    float at(float position) const
    {
        assert(position >= 0);
        assert(_data);

        int sample_pos = static_cast<int>(position);
        float weight = position - std::floor(position);
        float sample_low = (sample_pos < _length) ? _data[sample_pos] : 0.0f;
        float sample_high = (sample_pos + 1 < _length) ? _data[sample_pos + 1] : 0.0f;

        return (sample_high * weight + sample_low * (1.0f - weight));
    }

private:
    const float* _data{nullptr};
    int _length{0};
};

} // end namespace dsp
#endif //SUSHI_AUDIO_SAMPLE_H
