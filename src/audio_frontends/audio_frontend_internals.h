/**
 * @brief Common implementation details shared between audio frontends
 */
#ifndef SUSHI_AUDIO_FRONTEND_INTERNALS_H
#define SUSHI_AUDIO_FRONTEND_INTERNALS_H

#include <xmmintrin.h>

namespace sushi {
namespace audio_frontend {

inline void disable_denormals()
{
    _mm_setcsr(0x9FC0);
}

}; // end namespace audio_frontend
}; // end namespace sushi

#endif //SUSHI_AUDIO_FRONTEND_INTERNALS_H
