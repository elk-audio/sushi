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
 * @brief Compile time constants
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <chrono>

#ifndef SUSHI_CONSTANTS_H
#define SUSHI_CONSTANTS_H

#include <chrono>

/* The number of samples to process in one chunk. It is defined as a
compile-time constant to give more room for optimizations */
#ifdef SUSHI_CUSTOM_AUDIO_CHUNK_SIZE
constexpr int AUDIO_CHUNK_SIZE = SUSHI_CUSTOM_AUDIO_CHUNK_SIZE;
#else
constexpr int AUDIO_CHUNK_SIZE = 64;
#endif

constexpr int MAX_ENGINE_CV_IO_PORTS = 4;
constexpr int MAX_ENGINE_GATE_PORTS = 8;
constexpr int MAX_ENGINE_GATE_NOTE_NO = 127;

constexpr int MAX_TRACK_CHANNELS = 16;

constexpr float PAN_GAIN_3_DB = 1.412537f;
constexpr auto GAIN_SMOOTHING_TIME = std::chrono::milliseconds(20);

constexpr int SUSHI_PPQN_TICK = 24;

/* Use in class declaration to disallow copying of this class.
 * Note that this marks copy constructor and assignment operator
 * as deleted and hence their r-value counterparts are not generated.
 *
 * In order to make a class moveable though still non-copyable,
 * implement a move constructor and move assignment operator. Default
 * copy constructor will then not be generated. Usage of this macro is
 * in this case not neccesary to make the class non-copyable. But can
 * still be used for clarity.
 */
#define SUSHI_DECLARE_NON_COPYABLE(type) type(const type& other) = delete; \
                                         type& operator=(const type&) = delete;


#endif //SUSHI_CONSTANTS_H
