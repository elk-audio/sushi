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
 * @brief Wrapper for an audio sample to provide sample interpolation in an OO interface
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_AUDIO_SAMPLE_H
#define SUSHI_AUDIO_SAMPLE_H

#include <cmath>

namespace dsp {

/**
 * @brief Class to wrap a mono audio sample into a prettier interface
 */
class Sample
{
    SUSHI_DECLARE_NON_COPYABLE(Sample);
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
    float at(double position) const
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
#endif // SUSHI_AUDIO_SAMPLE_H
