#include "gtest/gtest.h"

#define private public
#define protected public

#include "engine/receiver.cpp"

using namespace sushi;
using namespace sushi::receiver;

constexpr auto ZERO_TIMEOUT = std::chrono::milliseconds(0);

class TestAsyncReceiver : public ::testing::Test
{
protected:
    TestAsyncReceiver()
    {
    }

    void SetUp()
    {}

    void TearDown()
    { }

    RtSafeRtEventFifo _queue;
    AsynchronousEventReceiver _module_under_test{&_queue};
};


TEST_F(TestAsyncReceiver, TestBasicHandling)
{
    ASSERT_FALSE(_module_under_test.wait_for_response(123u, ZERO_TIMEOUT));
    auto event = RtEvent::make_insert_processor_event(nullptr);
    EventId id = event.returnable_event()->event_id();
    event.returnable_event()->set_handled(true);
    _queue.push(event);
    ASSERT_TRUE(_module_under_test.wait_for_response(id, ZERO_TIMEOUT));
}

TEST_F(TestAsyncReceiver, TestMultipleEvents)
{
    ASSERT_FALSE(_module_under_test.wait_for_response(123u, ZERO_TIMEOUT));
    auto event1 = RtEvent::make_insert_processor_event(nullptr);
    auto event2 = RtEvent::make_add_processor_to_track_event(123, 234);
    event1.returnable_event()->set_handled(true);
    event2.returnable_event()->set_handled(true);
    EventId id1 = event1.returnable_event()->event_id();
    EventId id2 = event2.returnable_event()->event_id();
    _queue.push(event1);
    _queue.push(event2);
    // Get the acks in the reverse order to exercise more of the code
    ASSERT_TRUE(_module_under_test.wait_for_response(id2, ZERO_TIMEOUT));
    ASSERT_TRUE(_module_under_test.wait_for_response(id1, ZERO_TIMEOUT));
}