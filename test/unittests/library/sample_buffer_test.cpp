#include <algorithm>
#include <array>
#include "gtest/gtest.h"

#include "library/sample_buffer.h"
#include "test_utils/test_utils.h"

using namespace sushi;

TEST(TestSampleBuffer, TestCopying)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE, 2.0f);
    SampleBuffer<AUDIO_CHUNK_SIZE> copy_buffer(test_buffer);
    EXPECT_EQ(test_buffer.channel_count(), copy_buffer.channel_count());
    EXPECT_FLOAT_EQ(test_buffer.channel(0)[10], copy_buffer.channel(0)[10]);
    EXPECT_NE(test_buffer.channel(0), copy_buffer.channel(0));

    // When copy constructing from r-value, the original data should be preserved in a new container
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

    // Test assignment of empty buffers
    SampleBuffer<AUDIO_CHUNK_SIZE> empty_buffer;
    SampleBuffer<AUDIO_CHUNK_SIZE> empty_test_buffer = empty_buffer;

    EXPECT_EQ(empty_buffer.channel_count(), empty_test_buffer.channel_count());
    EXPECT_EQ(nullptr, empty_buffer.channel(0));

    // Assign empty buffer to non-empty buffer
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer_3(2);
    test_buffer_3 = empty_buffer;
    EXPECT_EQ(0, test_buffer_3.channel_count());
    EXPECT_EQ(nullptr, test_buffer_3.channel(0));

}

TEST(TestSampleBuffer, TestNonOwningBuffer)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer(4);
    float* data = test_buffer.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 2, 2.0f);
    std::fill(data + AUDIO_CHUNK_SIZE * 2, data + AUDIO_CHUNK_SIZE * 4, 4.0f);
    {
        // Create a non owning buffer and assert that is wraps the same data
        // And doesn't destroy the data when it goes out of scope
        SampleBuffer<AUDIO_CHUNK_SIZE> non_owning_buffer = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer, 0, 2);
        test_utils::assert_buffer_value(2.0f, non_owning_buffer);
        non_owning_buffer = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer, 2, 2);
        test_utils::assert_buffer_value(4.0f, non_owning_buffer);
        // Excercise assignment and move contructors
        SampleBuffer<AUDIO_CHUNK_SIZE> new_buffer(2);
        SampleBuffer<AUDIO_CHUNK_SIZE> new_buffer_2;
        new_buffer = non_owning_buffer;
        new_buffer_2 = std::move(non_owning_buffer);
    }
    // Touch the sample data to provoke a crash if it was accidentally deleted
    EXPECT_FLOAT_EQ(2.0f, *test_buffer.channel(1));
}

TEST(TestSampleBuffer, TestCreateFromRawPointer)
{
    std::array<float, 2 * AUDIO_CHUNK_SIZE> raw_data;
    std::fill(raw_data.begin(), raw_data.begin() + AUDIO_CHUNK_SIZE, 2.0f);
    std::fill(raw_data.begin() + AUDIO_CHUNK_SIZE, raw_data.end(), 4.0f);

    auto test_buffer = SampleBuffer<AUDIO_CHUNK_SIZE>::create_from_raw_pointer(raw_data.data(), 0, 2);
    EXPECT_EQ(2, test_buffer.channel_count());
    EXPECT_FLOAT_EQ(2.0, test_buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(4.0, test_buffer.channel(1)[0]);

    test_buffer = SampleBuffer<AUDIO_CHUNK_SIZE>::create_from_raw_pointer(raw_data.data(), 1, 1);
    EXPECT_EQ(1, test_buffer.channel_count());
    EXPECT_FLOAT_EQ(4.0, test_buffer.channel(0)[0]);
}


TEST(TestSampleBuffer, TestAssigningNonOwningBuffer)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer_1(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> test_buffer_2(2);

    float* data = test_buffer_1.channel(0);
    std::fill(data, data + AUDIO_CHUNK_SIZE * 2, 2.0f);
    test_buffer_2.clear();
    {
        // Create 2 non-owning buffers and assign one to the other
        SampleBuffer<AUDIO_CHUNK_SIZE> no_buffer_1 = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer_1, 0, 2);
        SampleBuffer<AUDIO_CHUNK_SIZE> no_buffer_2 = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer_2, 0, 2);
        test_utils::assert_buffer_value(2.0f, no_buffer_1);

        no_buffer_2 = no_buffer_1;
        test_utils::assert_buffer_value(2.0f, no_buffer_2);
        test_utils::assert_buffer_value(2.0f, test_buffer_2);
    }
    {
        // Copy construct 2 non-owning buffers and assign one to the other
        SampleBuffer<AUDIO_CHUNK_SIZE> no_buffer_1{SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer_1, 0, 2)};
        SampleBuffer<AUDIO_CHUNK_SIZE> no_buffer_2{SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(test_buffer_2, 0, 2)};
        test_utils::assert_buffer_value(2.0f, no_buffer_1);

        no_buffer_2.clear();
        no_buffer_2 = no_buffer_1;
        test_utils::assert_buffer_value(2.0f, no_buffer_2);
        test_utils::assert_buffer_value(2.0f, test_buffer_2);
    }
    // Touch the sample data to provoke a crash if it was accidentally deleted
    EXPECT_FLOAT_EQ(2.0f, *test_buffer_2.channel(1));
}

TEST(TestSampleBuffer, TestSwap)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_1(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(1);
    std::fill(buffer_1.channel(0), buffer_1.channel(0) + AUDIO_CHUNK_SIZE, 2.0f);

    swap(buffer_1, buffer_2);

    EXPECT_EQ(1, buffer_1.channel_count());
    EXPECT_EQ(2, buffer_2.channel_count());
    EXPECT_FLOAT_EQ(0.0f, buffer_1.channel(0)[0]);
    EXPECT_FLOAT_EQ(2.0f, buffer_2.channel(0)[0]);
}

TEST(TestSampleBuffer, TestInitialization)
{
    SampleBuffer<2> buffer(42);
    EXPECT_EQ(42, buffer.channel_count());
    SampleBuffer<3> buffer2;
    EXPECT_EQ(0, buffer2.channel_count());
    EXPECT_EQ(nullptr, buffer2.channel(0));
}

TEST(TestSampleBuffer, TestDeinterleaving)
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

TEST(TestSampleBuffer, TestInterleaving)
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
        ASSERT_FLOAT_EQ(0.0f, interleaved_buffer[n]);
        ASSERT_FLOAT_EQ(1.0f, interleaved_buffer[n+1]);
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


TEST (TestSampleBuffer, TestGain)
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
        ASSERT_FLOAT_EQ(4.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(6.0f, buffer.channel(1)[n]);
    }

    // Test per-channel gain
    buffer.apply_gain(1.5f, 0);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(6.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(6.0f, buffer.channel(1)[n]);
    }
}

TEST(TestSampleBuffer,TestReplace)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_1(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);
    test_utils::fill_sample_buffer(buffer_2, 2.0f);

    // copy ch 1 of buffer 2 to ch 0 of buffer_1
    buffer_1.replace(0, 1, buffer_2);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(2.0f, buffer_1.channel(0)[n]);
        ASSERT_FLOAT_EQ(1.0f, buffer_1.channel(1)[n]);
    }
    // copy all channels of buffer 2 to buffer 1
    buffer_1.replace(buffer_2);
    test_utils::assert_buffer_value(2.0f, buffer_1);
}

TEST (TestSampleBuffer, TestAdd)
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
        ASSERT_FLOAT_EQ(3.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(4.0f, buffer.channel(1)[n]);
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
        ASSERT_FLOAT_EQ(5.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(6.0f, buffer.channel(1)[n]);
    }
}

TEST (TestSampleBuffer, TestAddWithGain)
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
        ASSERT_FLOAT_EQ(4.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(5.0f, buffer.channel(1)[n]);
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
        ASSERT_FLOAT_EQ(7.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(8.0f, buffer.channel(1)[n]);
    }

    // Test single channel adding with gain
    buffer.add_with_gain(1, 1, buffer_2, -2.0f);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        ASSERT_FLOAT_EQ(7.0f, buffer.channel(0)[n]);
        ASSERT_FLOAT_EQ(6.0f, buffer.channel(1)[n]);
    }
}

TEST (TestSampleBuffer, TestRamping)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    for (unsigned int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
    {
        buffer.channel(0)[i] = 1.0f;
        buffer.channel(1)[i] = 1.0f;
    }
    buffer.ramp(1, 2);
    ASSERT_FLOAT_EQ(1.0f, buffer.channel(0)[0]);
    ASSERT_FLOAT_EQ(1.0f, buffer.channel(1)[0]);

    ASSERT_NEAR(2.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1], 0.0001f);
    ASSERT_NEAR(2.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1], 0.0001f);

    ASSERT_NEAR(1.5f, buffer.channel(0)[AUDIO_CHUNK_SIZE / 2], 0.05f);
    ASSERT_NEAR(1.5f, buffer.channel(1)[AUDIO_CHUNK_SIZE / 2], 0.05f);

    buffer.ramp_down();
    ASSERT_FLOAT_EQ(1.0f, buffer.channel(0)[0]);
    ASSERT_FLOAT_EQ(1.0f, buffer.channel(1)[0]);
    ASSERT_NEAR(0.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1], 0.0001f);
    ASSERT_NEAR(0.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1], 0.0001f);
}

TEST (TestSampleBuffer, TestAddWithRamp)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer_2(2);
    for (unsigned int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
    {
        buffer.channel(0)[i] = 1.0f;
        buffer.channel(1)[i] = 1.0f;
        buffer_2.channel(0)[i] = 1.0f;
        buffer_2.channel(1)[i] = 1.0f;
    }

    // Test buffers with equal channel count
    buffer.add_with_ramp(buffer_2, 0.5f, 1.0f);

    ASSERT_FLOAT_EQ(1.5f, buffer.channel(0)[0]);
    ASSERT_FLOAT_EQ(1.5f, buffer.channel(1)[0]);

    ASSERT_NEAR(1.75f, buffer.channel(0)[AUDIO_CHUNK_SIZE / 2 - 1], 0.05f);
    ASSERT_NEAR(1.75f, buffer.channel(1)[AUDIO_CHUNK_SIZE / 2 - 1], 0.05f);

    ASSERT_FLOAT_EQ(2.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    ASSERT_FLOAT_EQ(2.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);

    // Test adding mono buffer to stereo buffer with ramp
    SampleBuffer<AUDIO_CHUNK_SIZE> mono_buffer(1);
    for (unsigned int n = 0; n < AUDIO_CHUNK_SIZE; ++n)
    {
        mono_buffer.channel(0)[n] = 1.0f;
    }
    for (unsigned int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
    {
        buffer.channel(0)[i] = 1.0f;
        buffer.channel(1)[i] = 1.0f;
    }

    buffer.add_with_ramp(mono_buffer, 1.0f, 2.0f);

    ASSERT_FLOAT_EQ(2.0f, buffer.channel(0)[0]);
    ASSERT_FLOAT_EQ(2.0f, buffer.channel(1)[0]);

    ASSERT_NEAR(2.5f, buffer.channel(0)[AUDIO_CHUNK_SIZE / 2 - 1], 0.05f);
    ASSERT_NEAR(2.5f, buffer.channel(1)[AUDIO_CHUNK_SIZE / 2 - 1], 0.05f);

    ASSERT_FLOAT_EQ(3.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    ASSERT_FLOAT_EQ(3.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);
}

TEST (TestSampleBuffer, TestCountClippedSamples)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    ASSERT_EQ(0, buffer.count_clipped_samples(0));

    buffer.channel(0)[4] = 1.7f;
    buffer.channel(1)[3] = 1.1f;
    buffer.channel(1)[2] = -1.05f;
    EXPECT_EQ(1, buffer.count_clipped_samples(0));
    EXPECT_EQ(2, buffer.count_clipped_samples(1));
}

TEST (TestSampleBuffer, TestPeakCalculation)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    ASSERT_FLOAT_EQ(0, buffer.calc_peak_value(0));

    buffer.channel(0)[4] = 0.5f;
    buffer.channel(1)[3] = 1.1f;
    buffer.channel(1)[2] = -1.5f;
    EXPECT_FLOAT_EQ(0.5, buffer.calc_peak_value(0));
    EXPECT_FLOAT_EQ(1.5, buffer.calc_peak_value(1));
}

TEST (TestSampleBuffer, TestRMSCalculation)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> buffer(2);
    ASSERT_FLOAT_EQ(0, buffer.calc_rms_value(0));

    // Fill channel 0 with a square wave and channel 1 with a sine wave
    for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
    {
        buffer.channel(0)[i] = i % 2 == 0? 1.0f : -1.0f;
        buffer.channel(1)[i] = sinf(i * 0.5f);
    }

    EXPECT_FLOAT_EQ(1, buffer.calc_rms_value(0));
    EXPECT_NEAR(1.0f / std::sqrt(2), buffer.calc_rms_value(1), 0.01);
}
