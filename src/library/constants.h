/**
 * @Brief Compile time constants
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_CONSTANTS_H
#define SUSHI_CONSTANTS_H

/* The number of samples to process in one chunk. It is defined as a
compile time constant to give more room for optimizations */
static constexpr int AUDIO_CHUNK_SIZE = 64;

#endif //SUSHI_CONSTANTS_H
