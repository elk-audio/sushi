/**
 * @brief Common implementation details shared between audio frontends
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
