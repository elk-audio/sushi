/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Class to be used for hard limitng audio signals
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SAFTEY_LIMITER_H
#define SUSHI_SAFTEY_LIMITER_H

#include <array>
#include <cmath>

namespace dsp
{

/**
 * Calculated using windowed sinc method. Basic tests in python 
 * seemed to get good results with low latency. Might not be sufficient to
 * listen to but works for true peak detection
 */

constexpr float filter_coeffs[4][4] = {
    { -0.06615947186946869, 0.19239433109760284, 0.9733920693397522, -1.6899518229251953e-08,},
    { -0.09243691712617874, 0.4796152412891388, 0.779610812664032, -0.08357855677604675,},
    { -0.08357856422662735, 0.779610812664032, 0.4796152114868164, -0.09243690967559814,},
    { -1.6899520005608792e-08, 0.973392128944397, 0.19239431619644165, -0.06615947186946869,}
};

constexpr float THRESHOLD_DB = 0.0;
constexpr float RELEASE_TIME_MS = 100.0;

/**
 * @brief Brick wall "ear-saving" limiter. Stops the signal from ever exceeding 0.0 dB.
 * Instant attack with true peak detection. Could cause distortion in the "attack" portion
 * of a signal.
 * 
 */
class SafteyLimiter
{
public:
    /**
     * @brief 4x polyphase interpolator.
     * 
     */
    class UpSampler
    {
    public:
        UpSampler() {}

        /**
         * @brief Reset the interpolator for when processing is about to start
         * 
         */
        void reset()
        {
            _delay_line.fill(0.0f);
        }

        /**
         * @brief Interpolate one sample to 4x the original sample_rate
         * using a polyphase implementation
         * 
         * @param sample The sample to interpolate
         * @return std::array<float, 4> Resulting samples
         */
        inline std::array<float, 4> process_sample(float sample)
        {
            std::array<float, 4> output = {0.0, 0.0, 0.0, 0.0};
            _delay_line[_write_idx] = sample; // Write sample into internal delayline
            for (size_t i = 0; i < output.size(); i++)
            {
                float upsampled_value = 0.0;
                // Convolve the filter with the sample data
                for (size_t j = 0; j < _delay_line.size(); j++)
                {
                    int read_idx = (_write_idx - j) & 0b11;
                    upsampled_value += filter_coeffs[i][j] * _delay_line[read_idx];
                }
                output[i] = upsampled_value;
            }
            _write_idx = (_write_idx + 1) & 0b11; // Fast index wrapping for 2^n size circular buffers
            return output;
        }
    private:
        std::array<float, 4> _delay_line;
        int _write_idx{0};
    };

    SafteyLimiter(float release_time_ms = RELEASE_TIME_MS) : _release_time(release_time_ms) {}

    /**
     * @brief Recalculate release time based on sample rate and reset gain reduction
     * and up sampler.
     * 
     * @param sample_rate 
     */
    void prepare_to_play(float sample_rate)
    {
        _release_coeff = std::exp(-1.0f / (0.001f * sample_rate * _release_time));
        _gain_reduction = 0.0f;
        _up_sampler.reset();
    }

    /**
     * @brief Process audio limiting to output to maxmimum 0.0 dB
     * 
     * @param input array of input values
     * @param output array of output values
     * @param n_samples number of samples to process
     */
    void process(const float* input, float* output, int n_samples)
    {
        for (int sample_idx = 0; sample_idx < n_samples; sample_idx++)
        {
            // Calculate the highest peak from true peak calculations and the current sample value
            float true_peak = std::abs(input[sample_idx]);
            auto up_sampled_values = _up_sampler.process_sample(input[sample_idx]);
            for (auto sample : up_sampled_values)
            {
                true_peak = std::max(true_peak, std::abs(sample));
            }
            
            // Calculate gain reduction
            float true_peak_db = 20.0 * std::log10(true_peak);
            if (true_peak_db > THRESHOLD_DB)
            {
                _gain_reduction = std::max(_gain_reduction, 1.0f - 1.0f / true_peak);
            }
            else
            {
                _gain_reduction *= _release_coeff;
            }
            output[sample_idx] = input[sample_idx] * (1.0f - _gain_reduction);
        }
    }

private:
    float _gain_reduction{0.0};
    float _release_time{0.0};
    float _release_coeff{0.0};

    UpSampler _up_sampler;
};

} // namespace dsp


#endif // SUSHI_SAFTEY_LIMITER_H
