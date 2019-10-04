/**
 * @Brief Helper and utility functions for unit tests
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_TEST_UTILS_H
#define SUSHI_TEST_UTILS_H

#include "library/sample_buffer.h"

using ::testing::internal::posix::GetEnv;
using namespace sushi;
namespace test_utils {

// Enough leeway to approximate 6dB to 2 times amplification.
const float DECIBEL_ERROR = 0.01;

template <int size>
inline void fill_sample_buffer(SampleBuffer <size> &buffer, float value)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        std::fill(buffer.channel(ch), buffer.channel(ch) + size, value);
    }
};

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
};

template <int size>
inline void assert_buffer_value(float value, const SampleBuffer <size> &buffer, float error_margin)
{
    for (int ch = 0; ch < buffer.channel_count(); ++ch)
    {
        for (int i = 0; i < size; ++i)
        {
            ASSERT_NEAR(value, buffer.channel(ch)[i], error_margin);
        }
    }
};

inline std::string get_data_dir_path()
{
    // DO NOT COMMIT - JUST FOR GETTING TESTS TO RUN ON CLION.
    // Make general fix instead.
    //char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    char const* test_data_dir = "/home/ilias/workspaces/sushi/test/data";
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }
    std::string test_config_file(test_data_dir);
    test_config_file.append("/");
    return test_config_file;
};

// Macro to hide unused variable warnings when using structured bindings
#define DECLARE_UNUSED(var) [[maybe_unused]] auto unused_##var = var

} // namespace test_utils


#endif //SUSHI_TEST_UTILS_H
