//
// Created by gustav on 10/18/16.
//

#ifndef SUSHI_CONSTANTS_H
#define SUSHI_CONSTANTS_H

/* The number of samples to process in one chunk. It is defined as a
compile time constant to give more room for optimizations */
static constexpr unsigned int AUDIO_CHUNK_SIZE = 64;

#endif //SUSHI_CONSTANTS_H
