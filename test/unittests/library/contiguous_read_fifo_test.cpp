#include "gtest/gtest.h"

#include "library/contiguous_read_fifo.h"

using namespace sushi;

namespace {
    constexpr int TEST_FIFO_CAPACITY = 128;
    constexpr int TEST_DATA_SIZE = 100;
} // anonymous namespace

class TestContiguousReadFIFO : public ::testing::Test
{
protected:
    TestContiguousReadFIFO()
    {
    }

    void SetUp()
    {
        // Pre-fill queue
        for (int i=0; i<TEST_DATA_SIZE; i++)
        {
            ASSERT_EQ(true, _module_under_test.push(i));
        }
    }

    void TearDown()
    {
    }

    ContiguousReadFIFO<int, TEST_FIFO_CAPACITY> _module_under_test;
};

TEST_F(TestContiguousReadFIFO, test_non_overflowing_behaviour)
{

    auto read_buf = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, read_buf.n_items);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        EXPECT_EQ(i, read_buf.data[i]);
    }
}

TEST_F(TestContiguousReadFIFO, test_flush)
{
    _module_under_test.flush();
    auto read_buf = _module_under_test.flush();
    EXPECT_EQ(0, read_buf.n_items);
}

TEST_F(TestContiguousReadFIFO, test_overflow)
{
    constexpr int overflow_offset = 1000;

    for (int i=TEST_DATA_SIZE; i<TEST_FIFO_CAPACITY; i++)
    {
        ASSERT_EQ(true, _module_under_test.push(i));
    }
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        ASSERT_EQ(false, _module_under_test.push(overflow_offset + i));
    }
    auto read_buf = _module_under_test.flush();
    ASSERT_EQ(TEST_FIFO_CAPACITY, read_buf.n_items);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        EXPECT_EQ(overflow_offset + i, read_buf.data[i]);
    }
}

TEST_F(TestContiguousReadFIFO, test_flush_after_overflow)
{
    // Let the queue overflow...
    for (int i=0; i<(2*TEST_FIFO_CAPACITY); i++)
    {
        _module_under_test.push(i);
    }
    _module_under_test.flush();

    // ... and check that after flushing is working again in normal, non-overfowed conditions
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        ASSERT_EQ(true, _module_under_test.push(i));
    }
    auto read_buf = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, read_buf.n_items);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        EXPECT_EQ(i, read_buf.data[i]);
    }
}


