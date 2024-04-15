#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "library/fixed_stack.h"

using namespace sushi;

constexpr int STACK_SIZE = 5;

class TestFixedStack : public ::testing::Test
{
protected:
    TestFixedStack() = default;

    FixedStack<int, STACK_SIZE> _module_under_test;

    FixedStackAccessor<int, STACK_SIZE> _accessor {_module_under_test};
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

    ASSERT_EQ(2, _accessor.data()[2]);
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

