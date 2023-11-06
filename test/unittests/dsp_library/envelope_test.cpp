#include "gtest/gtest.h"

#define private public

#include "dsp_library/envelopes.h"


using namespace sushi::dsp;

/* Test the envelope class */
class TestADSREnvelope : public ::testing::Test
{
protected:
    TestADSREnvelope() = default;

    void SetUp() override
    {
        _module_under_test.set_samplerate(100);
        _module_under_test.set_parameters(1, 1, 0.5, 1);
    }

    AdsrEnvelope _module_under_test;
};

TEST_F(TestADSREnvelope, TestNormalOperation)
{
    EXPECT_TRUE(_module_under_test.finished());
    _module_under_test.gate(true);
    EXPECT_FALSE(_module_under_test.finished());

    /* Test Attack phase */
    float level = _module_under_test.tick(50);
    EXPECT_NEAR(0.5f, level, 0.001);
    level = _module_under_test.tick(50);
    /* this should be around the maximum peak */
    EXPECT_NEAR(1.0f, level, 0.001);
    /* Move into the decay phase */
    level = _module_under_test.tick(50);
    EXPECT_NEAR(0.75f, level, 0.001);
    /* Now we should be in the sustain phase */
    level = _module_under_test.tick(200);
    EXPECT_FLOAT_EQ(0.5f, level);

    /* Set gate off and go to decay phase */
    _module_under_test.gate(false);
    level = _module_under_test.tick(50);
    EXPECT_NEAR(0.25f, level, 0.001);
    level = _module_under_test.tick(55);
    EXPECT_FLOAT_EQ(0.0f, level);
    EXPECT_TRUE(_module_under_test.finished());
}

TEST_F(TestADSREnvelope, TestParameterLimits)
{
    _module_under_test.set_parameters(0, 0, 0.5f, 0);
    EXPECT_TRUE(_module_under_test.finished());
    _module_under_test.gate(true);
    EXPECT_FALSE(_module_under_test.finished());
    /* Only 1 state transition per tick, so tick is called twice */
    float level = _module_under_test.tick(2);
    level = _module_under_test.tick(2);
    EXPECT_FLOAT_EQ(0.5f, level);

    /* Reset and test with tick(0) */
    _module_under_test.reset();
    level = _module_under_test.tick(0);
    EXPECT_FLOAT_EQ(0.0f, level);
    EXPECT_FLOAT_EQ(0.0f, _module_under_test.level());
}
