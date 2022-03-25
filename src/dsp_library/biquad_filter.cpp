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
 * @brief Biquad filter implementation
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "biquad_filter.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

namespace dsp {
namespace biquad {

const int TIME_CONSTANTS_IN_SMOOTHING_FILTER = 3;

inline float process_one_pole(const OnePoleCoefficients coefficients, const float input, float &z)
{
    z = coefficients.b0 * input + coefficients.a0 * z;
    return z;
}

void calc_biquad_peak(Coefficients& filter, float samplerate, float frequency, float q, float gain)
{
    double A = std::sqrt(gain); // Note that the dB to linear gain conversion is done in the parameters preprocessor
    double w0 = 2 * M_PI * frequency / samplerate;
    double w0_cos = std::cos(w0);
    double w0_sin = std::sin(w0);
    double alpha = 0.5 * w0_sin / q;
    double a0 = 1 + alpha / A;

    // Calculating normalized filter coefficients
    filter.a1 = static_cast<float>(-2.0f * w0_cos / a0);
    filter.a2 = static_cast<float>((1 - alpha / A) / a0);
    filter.b0 = static_cast<float>((1 + alpha * A) /a0);
    filter.b1 = filter.a1;
    filter.b2 = static_cast<float>((1 - alpha * A) / a0);
}

void calc_biquad_lowpass(Coefficients&  filter, float samplerate, float frequency)
{
    float w0 = 2 * M_PI * frequency / samplerate;
    float w0_cos = std::cos(w0);
    float w0_sin = std::sin(w0);
    float alpha = w0_sin;
    float a0 = 1 + alpha;

    // Calculating normalized filter coefficients
    filter.a1 = -2.0f * w0_cos / a0;
    filter.a2 = (1 - alpha) / a0;
    filter.b0 = (1 - w0_cos) * 2.0f / a0;
    filter.b1 = (1- w0_cos) / a0;
    filter.b2 = filter.b0;
}

BiquadFilter::BiquadFilter()
{
}

BiquadFilter::BiquadFilter(const Coefficients &coefficients) :
        _coefficient_targets(coefficients)
{
}

void BiquadFilter::reset()
{
    /* Clear everything that is time-dependant in the filters processing to
     * put the filter in a default state */
    _delay_registers = {0.0, 0.0};
    _coefficients = _coefficient_targets;
    std::fill(_smoothing_registers, _smoothing_registers + NUMBER_OF_BIQUAD_COEF, 0.0);
}

void BiquadFilter::set_smoothing(int buffer_size)
{
    /* Coefficient changes are smoothed through a one pole lowpass filter
     * with a time constant set to match a fixed number of samples
     * Since the frequency low and the actual cut off frequency not crucial,
     * we can get by without a bilinear transformation and simply
     * calculate a time constant from an analogue prototype filter instead. */

    _smoothing_coefficients.b0 = std::exp(-2 * M_PI * (1.0f / buffer_size) * TIME_CONSTANTS_IN_SMOOTHING_FILTER);
    _smoothing_coefficients.a0 = 1 - _smoothing_coefficients.b0;
}

void BiquadFilter::set_coefficients(const Coefficients &coefficients)
{
    _coefficient_targets = coefficients;
}

void BiquadFilter::process(const float *input, float *output, int samples)
{
    for (int n = 0; n < samples; n++)
    {
        // Process the coefficients through a one pole smoothing filter
        _coefficients.b0 = process_one_pole(_smoothing_coefficients, _coefficient_targets.b0, _smoothing_registers[0]);
        _coefficients.b1 = process_one_pole(_smoothing_coefficients, _coefficient_targets.b1, _smoothing_registers[1]);
        _coefficients.b2 = process_one_pole(_smoothing_coefficients, _coefficient_targets.b2, _smoothing_registers[2]);
        _coefficients.a1 = process_one_pole(_smoothing_coefficients, _coefficient_targets.a1, _smoothing_registers[3]);
        _coefficients.a2 = process_one_pole(_smoothing_coefficients, _coefficient_targets.a2, _smoothing_registers[4]);

        // process actual filter data
        float x = input[n];
        float y = _coefficients.b0 * x + _delay_registers.z1;
        _delay_registers.z1 = _coefficients.b1 * x - _coefficients.a1 * y + _delay_registers.z2;
        _delay_registers.z2 = _coefficients.b2 * x - _coefficients.a2 * y;
        output[n] = y;
    }
}
} // end namespace biquad
} // end namespace dsp