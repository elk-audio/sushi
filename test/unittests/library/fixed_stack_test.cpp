#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
ELK_POP_WARNING

#include "library/fixed_stack.h"

using namespace sushi;

constexpr int STACK_SIZE = 5;

class TestFixedStack : public ::testing::Test
{
protected:
    TestFixedStack() {}

    FixedStack<int, STACK_SIZE> _module_under_test;
};

TEST_F(TestFixedStack, TestPush)
{
    EXPECT_TRUE(_module_under_test.empty());

    for (int i = 0; i < STACK_SIZE; i++)
    {
        EXPECT_TRUE(_module_under_test.push(i));
        EXPECT_FALSE(_module_under_test.empty());
    }
    ASSERT_FALSE(_module_under_test.push(10));

    ASSERT_EQ(2, _module_under_test._data[2]);
}

TEST_F(TestFixedStack, TestPop)
{
    for (int i = 0; i < STACK_SIZE; i++)
    {
        EXPECT_TRUE(_module_under_test.push(i));
    }
    ASSERT_TRUE(_module_under_test.full());

    int i = 0;
    while (!_module_under_test.empty())
    {
        int val;
        ASSERT_TRUE(_module_under_test.pop(val));
        ASSERT_EQ(val, STACK_SIZE - i - 1);
        i++;
    }
}

