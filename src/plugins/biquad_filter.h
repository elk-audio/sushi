/**
 * @brief Biquad filter implementation
 * @copyright MIND Music Labs AB, Stockholm
 *
 * A general biquad filter implementation with coefficient smoothing
 */

#ifndef EQUALIZER_BIQUADFILTER_H
#define EQUALIZER_BIQUADFILTER_H

namespace biquad {

const int NUMBER_OF_BIQUAD_COEF = 5;

struct BiquadCoefficients
{
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
};

struct BiquadDelayRegisters
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
void calc_biquad_peak(biquad::BiquadCoefficients& filter, float samplerate, float frequency, float q, float gain);
void calc_biquad_lowpass(BiquadCoefficients* filter, float samplerate, float frequency);

/*
 * Filter class
 */
class BiquadFilter
{
public:
    BiquadFilter();

    BiquadFilter(const BiquadCoefficients &coefficients);

    ~BiquadFilter()
    { };

    /*
     * Resets the processing state
     */
    void reset();
    /*
     * Sets the parameters for smoothing filter changes
     */
    void set_smoothing(int buffer_size);

    void set_coefficients(const BiquadCoefficients &coefficients);

    void process(const float *input, float *output, int samples);

private:
    BiquadCoefficients _coefficients{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    BiquadCoefficients _coefficient_targets{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    BiquadDelayRegisters _delay_registers{0.0f, 0.0f};
    OnePoleCoefficients _smoothing_coefficients{0.0f, 0.0f};
    float _smoothing_registers[NUMBER_OF_BIQUAD_COEF]{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

} // end namespace biquad

#endif //EQUALIZER_BIQUADFILTER_H
