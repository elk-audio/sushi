/**
 * @brief Helper and utility functions for unit tests
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TEST_UTILS_H
#define SUSHI_TEST_UTILS_H

#include "library/sample_buffer.h"

#include <iomanip>

using ::testing::internal::posix::GetEnv;
using namespace sushi;
namespace test_utils {

// Enough leeway to approximate 6dB to 2 times amplification.
const float DECIBEL_ERROR = 0.01;

template <int size>
inline void fill_sample_buffer(SampleBuffer<size>& buffer, float value)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        std::fill(buffer.channel(ch), buffer.channel(ch) + size, value);
    }
}

template <int size>
inline void assert_buffer_value(float value, const SampleBuffer <size> &buffer)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        for (int i = 0; i < size; ++i)
        {
            ASSERT_FLOAT_EQ(value, buffer.channel(ch)[i]);
        }
    }
}

template <int size>
inline void assert_buffer_value(float value, const SampleBuffer<size>& buffer, float error_margin)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        for (int i = 0; i < size; ++i)
        {
            ASSERT_NEAR(value, buffer.channel(ch)[i], error_margin);
        }
    }
}

inline std::string get_data_dir_path()
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }
    std::string test_config_file(test_data_dir);
    test_config_file.append("/");
    return test_config_file;
}

template <int size>
inline void compare_buffers(const float static_array[][size], ChunkSampleBuffer& buffer, int channels, float error_margin = 0.0001f)
{
    for (int i = 0; i < channels; i++)
    {
        for (int j = 0; j < std::min(AUDIO_CHUNK_SIZE, size); j++)
        {
            ASSERT_NEAR(static_array[i][j], buffer.channel(i)[j], error_margin);
        }
    }
}

template <int size>
inline void compare_buffers(ChunkSampleBuffer& buffer_1, ChunkSampleBuffer& buffer_2, int channels, float error_margin = 0.0001f)
{
    for (int i = 0; i < channels; i++)
    {
        for (int j = 0; j < std::min(AUDIO_CHUNK_SIZE, size); j++)
        {
            ASSERT_NEAR(buffer_1.channel(i)[j], buffer_2.channel(i)[j], error_margin);
        }
    }
}

// Utility for creating static buffers such as those used in vst2/lv2_wrapper_test, by copying values from console.
template <int size>
inline void print_buffer(ChunkSampleBuffer& buffer, int channels)
{
    std::cout << std::scientific << std::setprecision(10);
    int print_i = 0;
    for (int i = 0; i < channels; i++)
    {
        for (int j = 0; j < std::min(AUDIO_CHUNK_SIZE, size); j++)
        {
            std::cout << buffer.channel(i)[j] << "f, ";
            print_i++;


            if(print_i == 4)
            {
                std::cout << std::endl;
                print_i = 0;
            }
        }

        std::cout << std::endl;
    }
}


// Macro to hide unused variable warnings when using structured bindings
#define DECLARE_UNUSED(var) [[maybe_unused]] auto unused_##var = var

} // namespace test_utils


#endif //SUSHI_TEST_UTILS_H
