/**
 * @brief Biquad filter implementation
 * @copyright MIND Music Labs AB, Stockholm
 *
 * A general biquad filter implementation with coefficient smoothing
 */

#ifndef EQUALIZER_BIQUADFILTER_H
#define EQUALIZER_BIQUADFILTER_H

namespace dsp {
namespace biquad {

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
} // end namespace biquad
} // end namespace dsp

#endif //EQUALIZER_BIQUADFILTER_H
