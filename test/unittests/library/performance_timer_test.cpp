#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
#define protected public
ELK_POP_WARNING

#include "library/performance_timer.cpp"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::performance;

constexpr auto TEST_PERIOD = std::chrono::microseconds(100);

performance::TimePoint virtual_wait(const performance::TimePoint& tp, int n)
{
    /* "Wait" by rewinding the timestamp, makes the test robust against
     * threading and scheduling issues */
    return tp - n * (TEST_PERIOD / 10);
}

void run_test_scenario(PerformanceTimer& timer)
{
    auto start = timer.start_timer();
    start = virtual_wait(start, 1);
    timer.stop_timer(start, 1);

    start = timer.start_timer();
    start = virtual_wait(start, 1);
    timer.stop_timer(start, 1);

    start = timer.start_timer();
    start = virtual_wait(start, 5);
    timer.stop_timer(start, 2);

    start = timer.start_timer();
    start = virtual_wait(start, 3);
    timer.stop_timer(start, 2);
}

class TestPerformanceTimer : public ::testing::Test
{
protected:
    TestPerformanceTimer() = default;

    void SetUp() override
    {
        _module_under_test.set_timing_period(TEST_PERIOD);
        /* Hack to store records while not using the worker thread */
        _module_under_test._enabled = true;
    }

    PerformanceTimer _module_under_test;
};


TEST_F(TestPerformanceTimer, TestOperation)
{
    run_test_scenario(_module_under_test);
    _module_under_test._update_timings();

    auto timings_1 = _module_under_test.timings_for_node(1);
    ASSERT_TRUE(timings_1.has_value());
    auto timings_2 = _module_under_test.timings_for_node(2);
    ASSERT_TRUE(timings_1.has_value());
    auto timings_467 = _module_under_test.timings_for_node(467);
    ASSERT_FALSE(timings_467.has_value());

    auto t1 = timings_1.value();
    auto t2 = timings_2.value();

    ASSERT_TRUE(t1.min_case > 0);
    ASSERT_TRUE(t1.avg_case > 0);
    ASSERT_GE(t1.max_case, t1.min_case);
    ASSERT_TRUE(t2.min_case > 0);
    ASSERT_TRUE(t2.avg_case > 0);
    ASSERT_GE(t2.max_case, t2.min_case);

    ASSERT_GE(t2.max_case, t1.max_case);
    ASSERT_GE(t2.avg_case, t1.avg_case);
}

TEST_F(TestPerformanceTimer, TestClearRecords)
{
    run_test_scenario(_module_under_test);
    _module_under_test._update_timings();

    ASSERT_TRUE(_module_under_test.clear_timings_for_node(2));
    ASSERT_FALSE(_module_under_test.clear_timings_for_node(467));

    auto timings = _module_under_test.timings_for_node(2);
    ASSERT_TRUE(timings.has_value());
    auto t = timings.value();

    ASSERT_FLOAT_EQ(0.0f, t.avg_case);
    ASSERT_FLOAT_EQ(100.0f, t.min_case);
    ASSERT_FLOAT_EQ(0.0f, t.max_case);

    _module_under_test.clear_all_timings();

    timings = _module_under_test.timings_for_node(1);
    ASSERT_TRUE(timings.has_value());
    t = timings.value();

    ASSERT_FLOAT_EQ(0.0f, t.avg_case);
    ASSERT_FLOAT_EQ(100.0f, t.min_case);
    ASSERT_FLOAT_EQ(0.0f, t.max_case);
}
