/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Class to be used for hard limitng audio signals
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_MASTER_LIMITER_H
#define SUSHI_MASTER_LIMITER_H

#include <array>
#include <cmath>

namespace sushi::dsp {

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
constexpr float THRESHOLD_GAIN = 1.0;
constexpr float RELEASE_TIME_MS = 100.0;
constexpr float ATTACK_TIME_MS = 0.0;
constexpr int UPSAMPLING_FACTOR = 4;
/**
 * Since exponentials never reach their target this constant is used
 * to set a higher target than the intended one. This is then reversed
 * when checking if we reached the correct level
 *
 * Experiments in numpy showed 1.6 has good correlation with the attack time for a range of settings
 *
 */
constexpr float ATTACK_RATIO = 1.6;
/**
 * @brief 4x polyphase interpolator.
 *
 */
template<int CHUNK_SIZE>
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
    inline void process(const float* input, float* output)
    {
        for (int sample_idx = 0; sample_idx < CHUNK_SIZE; sample_idx++)
        {
            _delay_line[_write_idx] = input[sample_idx]; // Write sample into internal delayline
            for (size_t i = 0; i < UPSAMPLING_FACTOR; i++)
            {
                float upsampled_value = 0.0;
                // Convolve the filter with the sample data
                for (size_t j = 0; j < _delay_line.size(); j++)
                {
                    int read_idx = (_write_idx - j) & 0b11;
                    upsampled_value += filter_coeffs[i][j] * _delay_line[read_idx];
                }
                output[UPSAMPLING_FACTOR * sample_idx + i] = upsampled_value;
            }
            _write_idx = (_write_idx + 1) & 0b11; // Fast index wrapping for 2^n size circular buffers
        }
    }
private:
    std::array<float, 4> _delay_line;
    int _write_idx{0};
};

/**
 * @brief Brick wall "ear-saving" limiter. Stops the signal from ever exceeding 0.0 dB.
 * Instant attack with true peak detection. Could cause distortion in the "attack" portion
 * of a signal.
 *
 */
template<int CHUNK_SIZE>
class MasterLimiter
{
public:

    MasterLimiter(float release_time_ms = RELEASE_TIME_MS,
                  float attack_time_ms = ATTACK_TIME_MS) : _release_time(release_time_ms),
                                                           _attack_time(attack_time_ms) {}

    /**
     * @brief Recalculate release time based on sample rate and reset gain reduction
     * and up sampler.
     *
     * @param sample_rate
     */
    void init(float sample_rate)
    {
        _release_coeff = _release_time > 0.0f ? std::exp(-1.0f / (0.001f * sample_rate * _release_time)) : 0.0f;
        _attack_coeff = _attack_time > 0.0f ? std::exp(-1.0f / (0.001f * sample_rate * _attack_time)) : 0.0f;
        _gain_reduction = 0.0f;
        _up_sampler.reset();
    }

    /**
     * @brief Process audio limiting to output to maximum 0.0 dB
     *
     * @param input array of input values
     * @param output array of output values
     * @param n_samples number of samples to process
     */
    void process(const float* input, float* output)
    {
        std::array<float, UPSAMPLING_FACTOR * CHUNK_SIZE> up_sampled_values;
        _up_sampler.process(input, up_sampled_values.data());
        for (int sample_idx = 0; sample_idx < CHUNK_SIZE; sample_idx++)
        {
            // Calculate the highest peak from true peak calculations and the current sample value
            float true_peak = std::abs(input[sample_idx]);
            for (int upsampled_idx = 0; upsampled_idx < UPSAMPLING_FACTOR; upsampled_idx++)
            {
                true_peak = std::max(true_peak, std::abs(up_sampled_values[UPSAMPLING_FACTOR * sample_idx + upsampled_idx]));
            }

            // Calculate gain reduction
            if (true_peak > THRESHOLD_GAIN)
            {
                _gain_reduction_target = std::max(_gain_reduction_target, (1.0f - 1.0f / true_peak) * ATTACK_RATIO);
            }

            if (_gain_reduction_target > _gain_reduction)
            {
                _gain_reduction = (_gain_reduction - _gain_reduction_target) * _attack_coeff + _gain_reduction_target;
                if (_gain_reduction >= _gain_reduction_target / ATTACK_RATIO)
                {
                    _gain_reduction_target = 0.0;
                }
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
    float _gain_reduction_target{0.0};
    float _release_time{0.0};
    float _release_coeff{0.0};
    float _attack_time{0.0};
    float _attack_coeff{0.0};

    UpSampler<CHUNK_SIZE> _up_sampler;
};

} // end namespace sushi::dsp


#endif // SUSHI_MASTER_LIMITER_H
