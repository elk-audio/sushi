/**
 * @Brief Helper and utility functions for unit tests
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_TEST_UTILS_H
#define SUSHI_TEST_UTILS_H

#include "library/sample_buffer.h"

using namespace sushi;
namespace test_utils {

// Enough leeway to approximate 6dB to 2 times amplification.
const float DECIBEL_ERROR = 0.01;


inline void fill_sample_buffer(SampleBuffer <AUDIO_CHUNK_SIZE> &buffer, float value)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        std::fill(buffer.channel(ch), buffer.channel(ch) + AUDIO_CHUNK_SIZE, value);
    }
};

inline void assert_buffer_value(float value, SampleBuffer <AUDIO_CHUNK_SIZE> &buffer)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
        {
            ASSERT_FLOAT_EQ(value, buffer.channel(ch)[i]);
        }
    }
};

inline void assert_buffer_value(float value, SampleBuffer <AUDIO_CHUNK_SIZE> &buffer, float error_margin)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
        {
            ASSERT_NEAR(value, buffer.channel(ch)[i], error_margin);
        }
    }
};

} // namespace test_utils


#endif //SUSHI_TEST_UTILS_H
