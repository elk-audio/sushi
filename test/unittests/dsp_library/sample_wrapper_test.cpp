#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "dsp_library/sample_wrapper.h"

using namespace dsp;

const float SAMPLE_DATA[] = {1.0f, 2.0f, 2.0f, 1.0f, 1.0f};
const int SAMPLE_DATA_LENGTH = sizeof(SAMPLE_DATA) / sizeof(float);

class TestSampleWrapper : public ::testing::Test
{
protected:
    TestSampleWrapper() = default;

    Sample _module_under_test{SAMPLE_DATA, SAMPLE_DATA_LENGTH};
};

TEST_F(TestSampleWrapper, TestSampleInterpolation)
{
    // Get exact sample values
    EXPECT_FLOAT_EQ(1.0f, _module_under_test.at(0.0f));
    EXPECT_FLOAT_EQ(2.0f, _module_under_test.at(1.0f));
    // Get interpolated values
    EXPECT_FLOAT_EQ(1.5f, _module_under_test.at(2.5f));
}
