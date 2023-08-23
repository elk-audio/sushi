#include "gtest/gtest.h"

#include "test_utils/test_utils.h"

#define private public
#define protected public
#include "engine/event_dispatcher.cpp"
#include "test_utils/engine_mockup.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::dispatcher;

constexpr float TEST_SAMPLE_RATE = 44100.0;
constexpr auto EVENT_PROCESS_WAIT_TIME = std::chrono::milliseconds(1);

bool completed = false;
int completion_status = 0;

void dummy_callback(void* /*arg*/, Event* /*event*/, int status)
{
    completed = true;
    completion_status = status;
}

int dummy_processor_callback(void* /*arg*/, EventId /*id*/)
{
    completed = true;
    return EventStatus::HANDLED_OK;
}

class DummyPoster : public EventPoster
{
public:
    int process(Event* /*event*/) override
    {
        _received = true;
        return EventStatus::HANDLED_OK;
    }

    bool event_received()
    {
        if (_received)
        {
            _received = false;
            return true;
        }
        return false;
    }

private:
    bool _received {false};
};

class TestEventDispatcher : public ::testing::Test
{
public:
    void crank_event_loop_once()
    {
        _module_under_test->_running = false;
        _module_under_test->_event_loop();
    }

protected:
    TestEventDispatcher() = default;

    void SetUp() override
    {
        _module_under_test = new EventDispatcher(&_test_engine,
                                                 &_in_rt_queue,
                                                 &_out_rt_queue);
    }

    void TearDown() override
    {
        _module_under_test->stop();
        delete _module_under_test;
    }

    EventDispatcher*    _module_under_test{};
    EngineMockup        _test_engine{TEST_SAMPLE_RATE};
    RtSafeRtEventFifo   _in_rt_queue;
    RtSafeRtEventFifo   _out_rt_queue;
    DummyPoster         _poster;
};

TEST_F(TestEventDispatcher, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    _module_under_test->run();
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);
    _module_under_test->stop();
}

TEST_F(TestEventDispatcher, TestRegisteringAndDeregistering)
{
    auto status = _module_under_test->subscribe_to_keyboard_events(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->subscribe_to_keyboard_events(&_poster);
    EXPECT_EQ(EventDispatcherStatus::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->subscribe_to_parameter_change_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->subscribe_to_parameter_change_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->subscribe_to_engine_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->subscribe_to_engine_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->unsubscribe_from_keyboard_events(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->unsubscribe_from_keyboard_events(&_poster);
    EXPECT_EQ(EventDispatcherStatus::UNKNOWN_POSTER, status);

    status = _module_under_test->unsubscribe_from_parameter_change_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->unsubscribe_from_parameter_change_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::UNKNOWN_POSTER, status);

    status = _module_under_test->unsubscribe_from_engine_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->unsubscribe_from_engine_notifications(&_poster);
    EXPECT_EQ(EventDispatcherStatus::UNKNOWN_POSTER, status);
}

TEST_F(TestEventDispatcher, TestFromRtEventNoteOnEvent)
{
    RtEvent rt_event = RtEvent::make_note_on_event(10, 0, 0, 50, 10.f);
    _in_rt_queue.push(rt_event);

    _module_under_test->subscribe_to_keyboard_events(&_poster);
    crank_event_loop_once();

    ASSERT_TRUE(_poster.event_received());
}

TEST_F(TestEventDispatcher, TestFromRtEventParameterChangeNotification)
{
    RtEvent rt_event = RtEvent::make_parameter_change_event(10, 0, 10, 5.f);
    _in_rt_queue.push(rt_event);
    crank_event_loop_once();

    // Just test that a parameter change was queued. More thorough testing of ParameterManager is done elsewhere
    ASSERT_FALSE(_module_under_test->_parameter_manager._parameter_change_queue.empty());
}

TEST_F(TestEventDispatcher, TestEngineNotificationForwarding)
{
    auto event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                               123, 234, IMMEDIATE_PROCESS);
    _module_under_test->post_event(std::move(event));

    _module_under_test->subscribe_to_engine_notifications(&_poster);
    crank_event_loop_once();

    ASSERT_TRUE(_poster.event_received());
}

TEST_F(TestEventDispatcher, TestCompletionCallback)
{
    auto event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                               123, 234, IMMEDIATE_PROCESS);
    event->set_completion_cb(dummy_callback, nullptr);
    completed = false;
    completion_status = 0;

    _module_under_test->post_event(std::move(event));
    crank_event_loop_once();

    //ASSERT_TRUE(_poster.event_received());
    ASSERT_TRUE(completed);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status);
}

TEST_F(TestEventDispatcher, TestAsyncCallbackFromProcessor)
{
    auto rt_event = RtEvent::make_async_work_event(dummy_processor_callback, 123, nullptr);
    EventId sending_ev_id = rt_event.async_work_event()->event_id();
    _in_rt_queue.push(rt_event);

    /* Run the process loop once to convert from RtEvent and send the event to the worker,
     * then run the workers process loop once to execute the event, finally run the
     * dispatchers process loop a second time and assert that what we ended up with is
     * an RtEvent containing a completion notification */
    crank_event_loop_once();
    _module_under_test->_worker._worker();
    crank_event_loop_once();

    ASSERT_TRUE(_module_under_test->_in_queue.empty());
    ASSERT_FALSE(_out_rt_queue.empty());
    _out_rt_queue.pop(rt_event);
    EXPECT_EQ(RtEventType::ASYNC_WORK_NOTIFICATION, rt_event.type());
    auto typed_event = rt_event.async_work_completion_event();
    EXPECT_EQ(EventStatus::HANDLED_OK, typed_event->return_status());
    EXPECT_EQ(sending_ev_id, typed_event->sending_event_id());
    EXPECT_EQ(123u, typed_event->processor_id());
}

class TestWorker : public ::testing::Test
{
public:
    void crank_event_loop_once()
    {
        _module_under_test._running = false;
        _module_under_test._worker();
    }

protected:
    TestWorker() = default;

    void TearDown() override
    {
        _module_under_test.stop();
    }

    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    Worker       _module_under_test {&_test_engine, _test_engine.event_dispatcher()};
};

TEST_F(TestWorker, TestEventQueueingAndProcessing)
{
    completed = false;
    completion_status = 0;
    auto event = std::make_unique<SetEngineTempoEvent>(120.0f, IMMEDIATE_PROCESS);
    event->set_completion_cb(dummy_callback, nullptr);
    auto status = _module_under_test.dispatch(std::move(event));
    ASSERT_EQ(EventStatus::QUEUED_HANDLING, status);
    ASSERT_FALSE(_module_under_test._queue.empty());
    crank_event_loop_once();
    ASSERT_TRUE(completed);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status);
}
