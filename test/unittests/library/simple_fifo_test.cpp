#include "gtest/gtest.h"

#include "library/simple_fifo.h"

using namespace sushi;
using namespace sushi::internal;

constexpr int FIFO_SIZE = 5;

class TestSimpleFifo : public ::testing::Test
{
protected:
    TestSimpleFifo() {}

    SimpleFifo<int, FIFO_SIZE + 1> _module_under_test;
};

TEST_F(TestSimpleFifo, TestOperation)
{
    EXPECT_TRUE(_module_under_test.empty());

    for (int i = 0; i < FIFO_SIZE; ++i)
    {
        EXPECT_TRUE(_module_under_test.push(i));
        EXPECT_FALSE(_module_under_test.empty());
    }
    // Queue should now be full
    ASSERT_FALSE(_module_under_test.push(10));

    // Spot checks
    ASSERT_EQ(2, _module_under_test[2]);
    ASSERT_EQ(4, _module_under_test[4]);

    int val = 0;
    while (_module_under_test.empty() == false)
    {
        const auto& i = _module_under_test.pop();
        ASSERT_EQ(val, i);
        val++;
    }

    ASSERT_TRUE(_module_under_test.empty());
}

TEST_F(TestSimpleFifo, TestClear)
{
    for (int i = 0; i < FIFO_SIZE; ++i)
    {
        EXPECT_TRUE(_module_under_test.push(i));
        EXPECT_FALSE(_module_under_test.empty());
    }
    _module_under_test.clear();
    EXPECT_TRUE(_module_under_test.empty());
}

TEST_F(TestSimpleFifo, TestPopAndPush)
{
    for (int i = 0; i < FIFO_SIZE; ++i)
    {
        EXPECT_TRUE(_module_under_test.push(i));
    }
    int value = 12345;
    EXPECT_TRUE(_module_under_test.pop(value));
    EXPECT_EQ(0, value);
    EXPECT_TRUE(_module_under_test.pop(value));
    EXPECT_EQ(1, value);

    // Push 2 more, should wrap around
    EXPECT_TRUE(_module_under_test.push(FIFO_SIZE));
    EXPECT_TRUE(_module_under_test.push(FIFO_SIZE + 1));

    int i = 2;
    EXPECT_EQ(2, _module_under_test[0]);
    // empty buffer and check that values are popped in order
    while (_module_under_test.pop(value))
    {
        EXPECT_EQ(i, value);
        i++;
    }
    EXPECT_TRUE(_module_under_test.empty());
}