#include "gtest/gtest.h"

#define private public

#include "dsp_library/value_smoother.h"

using namespace sushi;

constexpr float TEST_SAMPLE_RATE = 1000;
constexpr float TEST_TARGET_VALUE = 1;

template <typename T, int mode>
void test_common(ValueSmoother<T, mode>& module_under_test)
{
    module_under_test.set_direct(2.0);
    EXPECT_FLOAT_EQ(2.0, module_under_test.value());
    EXPECT_TRUE(module_under_test.stationary());
    EXPECT_FLOAT_EQ(2.0, module_under_test.next_value());

    module_under_test.set(TEST_TARGET_VALUE);
    EXPECT_FLOAT_EQ(2.0, module_under_test.value());
    EXPECT_FALSE(module_under_test.stationary());

    for (int i = 0; i < 5; ++i)
    {
        auto val = module_under_test.value();
        EXPECT_LT(module_under_test.next_value(), val);
    }
}

class ValueSmootherTest : public ::testing::Test
{
protected:
    ValueSmootherTest() {}

    void SetUp()
    {
        _module_under_test_filter.set_lag_time(std::chrono::milliseconds(5), TEST_SAMPLE_RATE);
        _module_under_test_ramp.set_lag_time(std::chrono::milliseconds(5), TEST_SAMPLE_RATE);
        _module_under_test_exp_ramp.set_lag_time(std::chrono::milliseconds(5), TEST_SAMPLE_RATE);
    }
    ValueSmootherFilter<float> _module_under_test_filter;
    ValueSmootherRamp<float> _module_under_test_ramp;
    ValueSmootherExpRamp<float> _module_under_test_exp_ramp;
 };


TEST_F(ValueSmootherTest, TestLinearFloat)
{
    test_common(_module_under_test_ramp);

    EXPECT_TRUE(_module_under_test_ramp.stationary());
    EXPECT_FLOAT_EQ(TEST_TARGET_VALUE, _module_under_test_ramp.value());
}

TEST_F(ValueSmootherTest, TestExpFloat)
{
    test_common(_module_under_test_filter);
    /* As the filter version approaches the target value asymptotically, it needs
     * to run a few more cycles before the value comes close enough */
    for (int i = 0; i < 25; ++i)
    {
        _module_under_test_filter.next_value();
    }
    EXPECT_TRUE(_module_under_test_filter.stationary());
    EXPECT_NEAR(TEST_TARGET_VALUE, _module_under_test_filter.value(), 0.001);
}


TEST_F(ValueSmootherTest, TestExponentialFloat)
{
    test_common(_module_under_test_exp_ramp);

    EXPECT_TRUE(_module_under_test_exp_ramp.stationary());
    EXPECT_FLOAT_EQ(TEST_TARGET_VALUE, _module_under_test_exp_ramp.value());
}