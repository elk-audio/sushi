/**
 * @brief Common implementation details shared between audio frontends
 */
#ifndef SUSHI_AUDIO_FRONTEND_INTERNALS_H
#define SUSHI_AUDIO_FRONTEND_INTERNALS_H

#include <xmmintrin.h>

namespace sushi {
namespace audio_frontend {

/**
 * @brief Sets the FTZ (flush denormals to zero) and DAC (denormals are zero) flags
 *        in the cpu to avoid performance hits of denormals in the audio thread
 */
inline void set_flush_denormals_to_zero()
{
    _mm_setcsr(0x9FC0);
}

}; // end namespace audio_frontend
}; // end namespace sushi

#endif //SUSHI_AUDIO_FRONTEND_INTERNALS_H
