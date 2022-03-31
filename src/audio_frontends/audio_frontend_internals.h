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
 * @brief Common implementation details shared between audio frontends
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#ifndef SUSHI_AUDIO_FRONTEND_INTERNALS_H
#define SUSHI_AUDIO_FRONTEND_INTERNALS_H

#ifdef __x86_64__
#include <xmmintrin.h>
#endif

namespace sushi {
namespace audio_frontend {

/* These are calculated theoretical correction factors from the Sika board and
 * provide a mapping that works fine for parameters and reasonable for pitch.
 * Pitch mapping that need extra precision will need separate calibration for
 * each cv port.
 * The CV_IN_CORR also accounts for the fact that the cv inputs are inverted
 * in the Sika board. Could eventually be fixed in the driver */

constexpr float CV_OUT_CORR = 0.987f;
constexpr float CV_IN_CORR = -1.449f;

/**
 * @brief Sets the FTZ (flush denormals to zero) and DAC (denormals are zero) flags
 *        in the cpu to avoid performance hits of denormals in the audio thread. This
 *        is only needed for x86 based machines as ARM machines have it disabled by
 *        default if vectorization is enabled.
 */
inline void set_flush_denormals_to_zero()
{
    #ifdef __x86_64__
    _mm_setcsr(0x9FC0);
    #endif
}

/**
 * @return Maps a sample from an audio input [-1, 1] range to cv range [0, 1]
 * @param audio An audio sample
 * @return a float remapped to a [0, 1] range.
 */
inline float map_audio_to_cv(float audio)
{
    return (audio + 1.0f) * 0.5f;
}

/**
 * @return Maps a sample from a cv input [0, 1] range to audio range [-1, 1]
 * @param cv A cv value
 * @return a float remapped to a [-1, 1] range.
 */
inline float map_cv_to_audio(float cv)
{
    return cv * 2.0f - 1.0f;
}

/**
 * @brief Helper function to do ramping of cv outputs that are updated once per
 *        audio chunk.
 * @param output A float array of size AUDIO_CHUNK_SIZE where the smoothed data will
 *               be copied to.
 * @param current_value The current value of the smoother
 * @param target_value The target value for the smoother
 * @return The new current value
 */
inline float ramp_cv_output(float* output, float current_value, float target_value)
{
    float inc = (target_value - current_value) / (AUDIO_CHUNK_SIZE - 1);
    for (int i = 0 ; i < AUDIO_CHUNK_SIZE; ++i)
    {
        output[i] = current_value + inc * i;
    }
    return target_value;
}

} // end namespace audio_frontend
} // end namespace sushi

#endif //SUSHI_AUDIO_FRONTEND_INTERNALS_H
