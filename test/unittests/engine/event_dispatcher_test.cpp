#include "gtest/gtest.h"

#include "test_utils/test_utils.h"

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
#define protected public
ELK_POP_WARNING

#include "engine/event_dispatcher.cpp"
#include "test_utils/engine_mockup.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::dispatcher;

constexpr float TEST_SAMPLE_RATE = 44100.0;
constexpr auto EVENT_PROCESS_WAIT_TIME = std::chrono::milliseconds(1);

bool completed_1 = false;
int completion_status_1 = EventStatus::NOT_HANDLED;
bool completed_2 = false;
int completion_status_2 = EventStatus::NOT_HANDLED;
int last_callback = 0;

void dummy_callback_1(void* /*arg*/, Event* /*event*/, int status)
{
    completed_1 = true;
    completion_status_1 = status;
    last_callback = 1;
}

void dummy_callback_2(void* /*arg*/, Event* /*event*/, int status)
{
    completed_2 = true;
    completion_status_2 = status;
    last_callback = 2;
}

int dummy_processor_callback(void* /*arg*/, EventId /*id*/)
{
    completed_1 = true;
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

    EventDispatcher*    _module_under_test = nullptr;
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
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->subscribe_to_keyboard_events(&_poster);
    EXPECT_EQ(Status::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->subscribe_to_parameter_change_notifications(&_poster);
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->subscribe_to_parameter_change_notifications(&_poster);
    EXPECT_EQ(Status::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->subscribe_to_engine_notifications(&_poster);
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->subscribe_to_engine_notifications(&_poster);
    EXPECT_EQ(Status::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->unsubscribe_from_keyboard_events(&_poster);
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->unsubscribe_from_keyboard_events(&_poster);
    EXPECT_EQ(Status::UNKNOWN_POSTER, status);

    status = _module_under_test->unsubscribe_from_parameter_change_notifications(&_poster);
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->unsubscribe_from_parameter_change_notifications(&_poster);
    EXPECT_EQ(Status::UNKNOWN_POSTER, status);

    status = _module_under_test->unsubscribe_from_engine_notifications(&_poster);
    EXPECT_EQ(Status::OK, status);
    status = _module_under_test->unsubscribe_from_engine_notifications(&_poster);
    EXPECT_EQ(Status::UNKNOWN_POSTER, status);
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
    event->set_completion_cb(dummy_callback_1, nullptr);
    completed_1 = false;
    completion_status_1 = 0;

    _module_under_test->post_event(std::move(event));
    crank_event_loop_once();

    ASSERT_TRUE(completed_1);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status_1);
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

TEST_F(TestEventDispatcher, TestEventProcessingOrder)
{
    auto event_1 = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                                1, 1, IMMEDIATE_PROCESS);
    event_1->set_completion_cb(dummy_callback_1, nullptr);
    completed_1 = false;
    completion_status_1 = EventStatus::NOT_HANDLED;

    _module_under_test->post_event(std::move(event_1));

    auto event_2 = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                                2, 2, IMMEDIATE_PROCESS);
    event_2->set_completion_cb(dummy_callback_2, nullptr);
    completed_2 = false;
    completion_status_2 = EventStatus::NOT_HANDLED;

    _module_under_test->post_event(std::move(event_2));

    crank_event_loop_once();

    ASSERT_TRUE(completed_1);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status_1);

    ASSERT_TRUE(completed_2);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status_2);

    ASSERT_EQ(last_callback, 2);
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
    completed_1 = false;
    completion_status_1 = 0;
    auto event = std::make_unique<SetEngineTempoEvent>(120.0f, IMMEDIATE_PROCESS);
    event->set_completion_cb(dummy_callback_1, nullptr);
    auto status = _module_under_test.dispatch(std::move(event));
    ASSERT_EQ(EventStatus::QUEUED_HANDLING, status);
    ASSERT_FALSE(_module_under_test._queue.empty());
    crank_event_loop_once();
    ASSERT_TRUE(completed_1);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status_1);
}
