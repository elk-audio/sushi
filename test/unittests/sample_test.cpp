#include "gtest/gtest.h"

// A simple test case
TEST (SampleTest, SimpleTestCase) {
    ASSERT_TRUE (1);
}

// A more complex test case where tests can be grouped
// And setup and teardown functions added.
class SampleTestCase : public ::testing::Test
{
protected:
    SampleTestCase() = default;

    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SampleTestCase, SampleTest)
{
    EXPECT_FALSE(0);
}
