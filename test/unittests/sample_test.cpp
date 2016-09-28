#include "gtest/gtest.h"
#define private public

// A simple test case
TEST (SampleTest, SimpleTestCase) {
    ASSERT_TRUE (1);
}

// A more complex test case where tests can be grouped
// And setup and teardown functions added.
class SampleTestCase : public ::testing::Test
{
    protected:
    SampleTestCase()
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(SampleTestCase, SampleTest)
{
    EXPECT_FALSE(0);
}
