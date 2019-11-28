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

}; // end namespace audio_frontend
}; // end namespace sushi

#endif //SUSHI_AUDIO_FRONTEND_INTERNALS_H
