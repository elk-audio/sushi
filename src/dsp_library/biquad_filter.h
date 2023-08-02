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
 * @brief Biquad filter implementation
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 *
 * A general biquad filter implementation with coefficient smoothing
 */

#ifndef EQUALIZER_BIQUADFILTER_H
#define EQUALIZER_BIQUADFILTER_H

namespace sushi::dsp::biquad {

const int NUMBER_OF_BIQUAD_COEF = 5;

struct Coefficients
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
};

struct DelayRegisters
{
    float z1;
    float z2;
};

struct OnePoleCoefficients
{
    float b0;
    float a0;
};


/*
 * Stand-alone functions for calculating coefficients
 */
void calc_biquad_peak(Coefficients &filter, float samplerate, float frequency, float q, float gain);

void calc_biquad_lowpass(Coefficients* filter, float samplerate, float frequency);

/*
 * Filter class
 */
class BiquadFilter
{
public:
    BiquadFilter();

    BiquadFilter(const Coefficients &coefficients);

    ~BiquadFilter() = default;

    /*
     * Resets the processing state
     */
    void reset();

    /*
     * Sets the parameters for smoothing filter changes
     */
    void set_smoothing(int buffer_size);

    void set_coefficients(const Coefficients &coefficients);

    void process(const float* input, float* output, int samples);

private:
    Coefficients _coefficients{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Coefficients _coefficient_targets{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    DelayRegisters _delay_registers{0.0f, 0.0f};
    OnePoleCoefficients _smoothing_coefficients{0.0f, 0.0f};
    float _smoothing_registers[NUMBER_OF_BIQUAD_COEF]{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

} // end namespace sushi::dsp::biquad

#endif //EQUALIZER_BIQUADFILTER_H
