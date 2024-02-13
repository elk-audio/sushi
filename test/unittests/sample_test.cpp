#include "gtest/gtest.h"

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
ELK_POP_WARNING

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
