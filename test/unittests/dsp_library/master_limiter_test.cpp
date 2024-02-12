#include <array>

#include "gtest/gtest.h"

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_WARNING (WARN_KEYWORD_MACRO)
#define private public
ELK_POP_WARNING

#include "dsp_library/master_limiter.h"
#include "test/data/master_limiter_test_data.h"

using namespace sushi::dsp;

class TestUpSampler : public ::testing::Test
{
protected:
    TestUpSampler() = default;
    void SetUp() override
    {
        _module_under_test.reset();
    }

    UpSampler<UPSAMPLING_TEST_DATA_SIZE> _module_under_test;
};

TEST_F(TestUpSampler, UpSampling)
{
    std::array<float, UPSAMPLING_TEST_DATA4X_SIZE> out;
    _module_under_test.process(UPSAMPLING_TEST_DATA, out.data());
    for (size_t i = 0; i < UPSAMPLING_TEST_DATA4X_SIZE; i++)
    {
        EXPECT_FLOAT_EQ(UPSAMPLING_TEST_DATA4X[i], out[i]);
    }
}

constexpr float TEST_SAMPLERATE = 48000.0f;
constexpr float TEST_RELEASE_TIME_MS = 100.0f;
constexpr float TEST_ATTACK_TIME_MS = 50.0f;

class TestMasterLimiter : public ::testing::Test
{
protected:
    TestMasterLimiter() = default;
    void SetUp() override
    {
        _module_under_test.init(TEST_SAMPLERATE);
    }

    MasterLimiter<LIMITER_INPUT_DATA_SIZE> _module_under_test{TEST_RELEASE_TIME_MS, TEST_ATTACK_TIME_MS};
};

TEST_F(TestMasterLimiter, Limit)
{
    float out[LIMITER_OUTPUT_DATA_SIZE];
    _module_under_test.process(LIMITER_INPUT_DATA, out);
    for (int i = 0; i < LIMITER_OUTPUT_DATA_SIZE; i++)
    {
        EXPECT_NEAR(1.0, out[i] / LIMITER_OUTPUT_DATA[i], 1e-6);
    }
}