#include <algorithm>

#include "gtest/gtest.h"
#include "test_utils.h"
#include "library/sample_buffer.h"

#define private public

using namespace sushi;

constexpr unsigned int SAMPLE_RATE = 44000;

TEST(TestSampleBuffer, TestCopying)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE, 2.0f);
    SampleBuffer<AUDIO_CHUNK_SIZE> copy_buffer(test_buffer);
    EXPECT_EQ(test_buffer.channel_count(), copy_buffer.channel_count());
    EXPECT_FLOAT_EQ(test_buffer.channel(0)[10], copy_buffer.channel(0)[10]);
    EXPECT_NE(test_buffer.channel(0), copy_buffer.channel(0));

    //When copy constructing from r-value, the original data should be preserved in a new container
    SampleBuffer<AUDIO_CHUNK_SIZE> r_value_copy(std::move(test_buffer));
    EXPECT_EQ(copy_buffer.channel_count(), r_value_copy.channel_count());
    EXPECT_FLOAT_EQ(copy_buffer.channel(0)[10], r_value_copy.channel(0)[10]);
    EXPECT_NE(copy_buffer.channel(0), r_value_copy.channel(0));
}

TEST(TestSampleBuffer, TestAssignement)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 4, 2.0f);
    SampleBuffer<AUDIO_CHUNK_SIZE> copy_buffer = test_buffer;

    EXPECT_EQ(test_buffer.channel_count(), copy_buffer.channel_count());
    EXPECT_FLOAT_EQ(test_buffer.channel(0)[10], copy_buffer.channel(0)[10]);
    EXPECT_NE(test_buffer.channel(0), copy_buffer.channel(0));

    // Test assigment that involves reallocation (from 4 channels to 2)
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer_2(2);
    data = test_buffer_2.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 2, 3.0f);
    copy_buffer = test_buffer_2;

    EXPECT_EQ(test_buffer_2.channel_count(), copy_buffer.channel_count());
    EXPECT_FLOAT_EQ(test_buffer_2.channel(0)[10], copy_buffer.channel(0)[10]);
    EXPECT_NE(test_buffer_2.channel(0), copy_buffer.channel(0));

    // Test move assigment, the original data should be preserved in a new container
    data = test_buffer.channel(0);
    SampleBuffer<AUDIO_CHUNK_SIZE> move_copy = std::move(test_buffer);
    EXPECT_EQ(4, move_copy.channel_count());
    EXPECT_FLOAT_EQ(2.0f, move_copy.channel(0)[10]);
    EXPECT_EQ(data, move_copy.channel(0));
}

TEST(TestSampleBuffer, test_initialization)
{
    SampleBuffer<2> buffer(42);
    EXPECT_EQ(42, buffer.channel_count());
    SampleBuffer<3> buffer2;
    EXPECT_EQ(0, buffer2.channel_count());
    EXPECT_EQ(nullptr, buffer2.channel(0));
}

TEST(TestSampleBuffer, test_deinterleave_buffer)
{
    float interleaved_buffer[6] = {1, 2, 1, 2, 1, 2};
    SampleBuffer<3> buffer(2);
    buffer.from_interleaved(interleaved_buffer);
    for (unsigned int n = 0; n < 3; ++n)
    {
        ASSERT_FLOAT_EQ(1.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(2.0f, buffer.channel(1)[n]);
    }

    float interleaved_3_ch[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};
    SampleBuffer<3> buffer_3ch(3);
    buffer_3ch.from_interleaved(interleaved_3_ch);
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
    buffer.to_interleaved(interleaved_buffer);
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
    buffer_3ch.to_interleaved(interleaved_3ch);
    for (unsigned int n = 0; n < (AUDIO_CHUNK_SIZE * 3); n+=3)
    {
        ASSERT_FLOAT_EQ(0.5f, interleaved_3ch[n]);
        ASSERT_FLOAT_EQ(1.0f, interleaved_3ch[n+1]);
        ASSERT_FLOAT_EQ(2.0f, interleaved_3ch[n+2]);
    }
}


TEST (TestSampleBuffer, test_gain)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        buffer.channel(0)[n] = 2.0f;
        buffer.channel(1)[n] = 3.0f;
    }

    // Test gain
    buffer.apply_gain(2.0f);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 4.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 6.0f);
    }

    // Test per-channel gain
    buffer.apply_gain(1.5f, 0);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 6.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 6.0f);
    }
}

TEST(TestSampleBuffer, test_replace)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_1(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);
    test_utils::fill_sample_buffer(buffer_2, 2.0f);

    // copy ch 1 of buffer 2 to ch 0 of buffer_1
    buffer_1.replace(0, 1, buffer_2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer_1.channel(0)[n], 2.0f);
        ASSERT_FLOAT_EQ(buffer_1.channel(1)[n], 1.0f);
    }
}

TEST (TestSampleBuffer, test_add)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        buffer.channel(0)[n] = 2.0f;
        buffer.channel(1)[n] = 3.0f;
        buffer_2.channel(0)[n] = 1.0f;
        buffer_2.channel(1)[n] = 1.0f;
    }
    // Test buffers with equal channel count
    buffer.add(buffer_2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 3.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 4.0f);
    }

    // Test adding mono buffer to stereo buffer
    SampleBuffer<AUDIO_CHUNK_SIZE> mono_buffer(1);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        mono_buffer.channel(0)[n] = 2.0f;
    }

    buffer.add(mono_buffer);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 5.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 6.0f);
    }
}

TEST (TestSampleBuffer, test_add_with_gain)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        buffer.channel(0)[n] = 2.0f;
        buffer.channel(1)[n] = 3.0f;
        buffer_2.channel(0)[n] = 1.0f;
        buffer_2.channel(1)[n] = 1.0f;
    }
    // Test buffers with equal channel count
    buffer.add_with_gain(buffer_2, 2.0f);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 4.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 5.0f);
    }

    // Test adding mono buffer to stereo buffer
    SampleBuffer<AUDIO_CHUNK_SIZE> mono_buffer(1);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        mono_buffer.channel(0)[n] = 2.0f;
    }

    buffer.add_with_gain(mono_buffer, 1.5f);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(buffer.channel(0)[n], 7.0f);
        ASSERT_FLOAT_EQ(buffer.channel(1)[n], 8.0f);
    }
}
