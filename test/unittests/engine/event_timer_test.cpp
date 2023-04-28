#include "gtest/gtest.h"

#define private public
#define protected public
#include "engine/event_timer.cpp"

using namespace sushi;
using namespace sushi::event_timer;
using namespace std::chrono_literals;

constexpr float TEST_SAMPLE_RATE = 44000.0f;

TEST(TestEventTimerInternal, TestCalcChunkTime)
{
    int microsecs = std::round(1000000.0f * AUDIO_CHUNK_SIZE / TEST_SAMPLE_RATE);
    auto chunk_time = calc_chunk_time(TEST_SAMPLE_RATE);
    ASSERT_EQ(microsecs, chunk_time.count());
}


class TestEventTimer : public ::testing::Test
{
protected:
    TestEventTimer()
    {
    }

    EventTimer       _module_under_test{TEST_SAMPLE_RATE};
};

TEST_F(TestEventTimer, TestToOffsetConversion)
{
    _module_under_test.set_incoming_time(1s);
    bool send_now;
    int offset;

    /* A timestamp far into the future should return false */
    std::tie(send_now, offset) = _module_under_test.sample_offset_from_realtime(3s);
    ASSERT_FALSE(send_now);

    /* A timestamp in the past should return true and offset 0 */
    std::tie(send_now, offset) = _module_under_test.sample_offset_from_realtime(0s);
    ASSERT_TRUE(send_now);
    ASSERT_EQ(0, offset);

    /* Create a timestamp in the middle of the chunk, note we must add
     * chunk time here because the EventTimer is 1 chunk ahead internally,
     * Because of rounding errors, the resulting sample offset could be
     * both AUDIO_CHUNK_SIZE / 2 and AUDIO_CHUNK_SIZE / 2 -1 */
    auto chunk_time = calc_chunk_time(TEST_SAMPLE_RATE);
    Time timestamp = 1s + chunk_time + chunk_time / 2;
    std::tie(send_now, offset) = _module_under_test.sample_offset_from_realtime(timestamp);
    ASSERT_TRUE(send_now);
    ASSERT_GE(offset, AUDIO_CHUNK_SIZE / 2 - 1);
    ASSERT_LE(offset, AUDIO_CHUNK_SIZE / 2);
}

TEST_F(TestEventTimer, TestToRealTimesConversion)
{
    auto chunk_time = calc_chunk_time(TEST_SAMPLE_RATE);
    _module_under_test.set_outgoing_time(1s);

    Time timestamp = _module_under_test.real_time_from_sample_offset(0);
    ASSERT_EQ((1s + chunk_time).count(), timestamp.count());

    timestamp = _module_under_test.real_time_from_sample_offset(AUDIO_CHUNK_SIZE / 2);
    ASSERT_EQ((1s + chunk_time + chunk_time / 2).count(), timestamp.count());
}