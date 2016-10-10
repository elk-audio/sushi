#include <algorithm>

#include "gtest/gtest.h"
#include "library/sample_buffer.h"

#define private public

constexpr unsigned int SAMPLE_RATE = 44000;

TEST(TestSampleBuffer, TestCopying)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE, 2.0f);
    SampleBuffer<AUDIO_CHUNK_SIZE> copy_buffer(test_buffer);
    ASSERT_EQ(test_buffer.channel_count(), copy_buffer.channel_count());
    ASSERT_FLOAT_EQ(test_buffer.channel(0)[10], copy_buffer.channel(0)[10]);
    ASSERT_NE(test_buffer.channel(0), copy_buffer.channel(0));

    //When copy constructing from r-value, the original data should be preserved in a new container
    SampleBuffer<AUDIO_CHUNK_SIZE> r_value_copy(std::move(test_buffer));
    ASSERT_EQ(copy_buffer.channel_count(), r_value_copy.channel_count());
    ASSERT_FLOAT_EQ(copy_buffer.channel(0)[10], r_value_copy.channel(0)[10]);
    ASSERT_NE(copy_buffer.channel(0), r_value_copy.channel(0));
}

TEST(TestSampleBuffer, TestAssignement)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 4, 2.0f);
    SampleBuffer<AUDIO_CHUNK_SIZE> copy_buffer = test_buffer;

    ASSERT_EQ(test_buffer.channel_count(), copy_buffer.channel_count());
    ASSERT_FLOAT_EQ(test_buffer.channel(0)[10], copy_buffer.channel(0)[10]);
    ASSERT_NE(test_buffer.channel(0), copy_buffer.channel(0));

    // Test assigment that involves reallocation (from 4 channels to 2)
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer_2(2);
    data = test_buffer_2.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 2, 3.0f);
    copy_buffer = test_buffer_2;

    ASSERT_EQ(test_buffer_2.channel_count(), copy_buffer.channel_count());
    ASSERT_FLOAT_EQ(test_buffer_2.channel(0)[10], copy_buffer.channel(0)[10]);
    ASSERT_NE(test_buffer_2.channel(0), copy_buffer.channel(0));

    // Test move assigment, the original data should be preserved in a new container
    data = test_buffer.channel(0);
    SampleBuffer<AUDIO_CHUNK_SIZE> move_copy = std::move(test_buffer);
    ASSERT_EQ(4, move_copy.channel_count());
    ASSERT_FLOAT_EQ(2.0f, move_copy.channel(0)[10]);
    ASSERT_EQ(data, move_copy.channel(0));
}

TEST(TestSampleBuffer, test_channel_count)
{
    SampleBuffer<2> buffer(42);
    ASSERT_EQ(42, buffer.channel_count());
    SampleBuffer<3> buffer2;
    ASSERT_EQ(0, buffer2.channel_count());
    ASSERT_EQ(nullptr, buffer2.channel(0));
}

TEST(TestSampleBuffer, test_deinterleave_buffer)
{
    float interleaved_buffer[6] = {1, 2, 1, 2, 1, 2};
    SampleBuffer<3> buffer(2);
    buffer.input_from_interleaved(interleaved_buffer);
    for (unsigned int n = 0; n < 3; ++n)
    {
        ASSERT_FLOAT_EQ(1.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(2.0f, buffer.channel(1)[n]);
    }

    float interleaved_3_ch[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};
    SampleBuffer<3> buffer_3ch(3);
    buffer_3ch.input_from_interleaved(interleaved_3_ch);
    for (unsigned int n = 0; n < 3; ++n)
    {
        ASSERT_FLOAT_EQ(1.0f, buffer_3ch.channel(0)[n]);
        ASSERT_FLOAT_EQ(2.0f, buffer_3ch.channel(1)[n]);
        ASSERT_FLOAT_EQ(3.0f, buffer_3ch.channel(2)[n]);
    }
}

TEST(TestSampleBuffer, test_interleave_buffer)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        buffer.channel(0)[n] = 0.0f;
        buffer.channel(1)[n] = 1.0f;
    }
    float interleaved_buffer[AUDIO_CHUNK_SIZE * 2];
    buffer.output_to_interleaved(interleaved_buffer);
    for (unsigned int n = 0; n < (AUDIO_CHUNK_SIZE * 2) ; n+=2)
    {
        ASSERT_FLOAT_EQ(interleaved_buffer[n], 0.0f);
        ASSERT_FLOAT_EQ(interleaved_buffer[n+1], 1.0f);
    }

    float interleaved_3ch[AUDIO_CHUNK_SIZE * 3];
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_3ch(3);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        buffer_3ch.channel(0)[n] = 0.5f;
        buffer_3ch.channel(1)[n] = 1.0f;
        buffer_3ch.channel(2)[n] = 2.0f;
    }
    buffer_3ch.output_to_interleaved(interleaved_3ch);
    for (unsigned int n = 0; n < (AUDIO_CHUNK_SIZE * 3); n+=3)
    {
        ASSERT_FLOAT_EQ(0.5f, interleaved_3ch[n]);
        ASSERT_FLOAT_EQ(1.0f, interleaved_3ch[n+1]);
        ASSERT_FLOAT_EQ(2.0f, interleaved_3ch[n+2]);
    }
}

