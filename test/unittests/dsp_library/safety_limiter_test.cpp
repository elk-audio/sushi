#include <array>

#include "gtest/gtest.h"

#define private public

#include "dsp_library/safety_limiter.h"
#include "test/data/safety_limiter_test_data.h"

using namespace dsp;

class TestUpSampler : public ::testing::Test
{
protected:
    TestUpSampler() {}
    void SetUp()
    {
        _module_under_test.reset();
    }

    UpSampler _module_under_test;
};

TEST_F(TestUpSampler, UpSampling)
{
    for (size_t i = 0; i < UPSAMPLING_TEST_DATA_SIZE; i++)
    {
        auto out = _module_under_test.process_sample(UPSAMPLING_TEST_DATA[i]);
        for (size_t j = 0; j < out.size(); j++)
        {
            EXPECT_FLOAT_EQ(UPSAMPLING_TEST_DATA4X[i * out.size() + j], out[j]);
        }
    }
}

constexpr float TEST_SAMPLERATE = 48000.0f;
constexpr float TEST_RELEASE_TIME_MS = 100.0f;

class TestSafetyLimiter : public ::testing::Test
{
protected:
    TestSafetyLimiter() {}
    void SetUp()
    {
        _module_under_test.prepare_to_play(48000.0);
    }

    SafetyLimiter _module_under_test{RELEASE_TIME_MS};
};

TEST_F(TestSafetyLimiter, Limit)
{
    float out[LIMITER_OUTPUT_DATA_SIZE];
    _module_under_test.process(LIMITER_INPUT_DATA, out, LIMITER_OUTPUT_DATA_SIZE);
    for (int i = 0; i < LIMITER_OUTPUT_DATA_SIZE; i++)
    {
        EXPECT_FLOAT_EQ(LIMITER_OUTPUT_DATA[i], out[i]);
    };
}

