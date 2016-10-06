#include <algorithm>

#include "gtest/gtest.h"
#include "library/sample_buffer.h"

#define private public

constexpr unsigned int SAMPLE_RATE = 44000;


TEST(TestSampleBuffer, test_deinterleave_buffer)
{
    float interleaved_buffer[AUDIO_CHUNK_SIZE * 2];
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE*2; n+=2)
    {
        interleaved_buffer[n] = 0.0f;
        interleaved_buffer[n+1] = 1.0f;
    }
    SampleBuffer buffer;
    buffer.input_from_interleaved(&interleaved_buffer[0]);

    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        ASSERT_FLOAT_EQ(0.0f, buffer.left_in[n]);
        ASSERT_FLOAT_EQ(1.0f, buffer.right_in[n]);
    }

}

TEST(TestSampleBuffer, test_interleave_buffer)
{
    SampleBuffer buffer;
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        buffer.left_out[n] = 0.0f;
        buffer.right_out[n] = 1.0f;
    }
    float interleaved_buffer[AUDIO_CHUNK_SIZE * 2];
    buffer.output_to_interleaved(&interleaved_buffer[0]);
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE*2; n+=2)
    {
        ASSERT_FLOAT_EQ(interleaved_buffer[n], 0.0f);
        ASSERT_FLOAT_EQ(interleaved_buffer[n+1], 1.0f);
    }
    buffer.input_from_interleaved(&interleaved_buffer[0]);
}

