#include "gtest/gtest.h"

#include "test_utils/test_utils.h"

#define private public
#define protected public
#include "engine/event_dispatcher.cpp"
#include "test_utils/engine_mockup.h"

using namespace sushi;
using namespace sushi::dispatcher;

constexpr int DUMMY_POSTER_ID = 1;
constexpr int DUMMY_STATUS = 100;
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
    return DUMMY_STATUS;
}

class DummyPoster : public EventPoster
{
public:
    int process(Event* /*event*/) override
    {
        _received = true;
        return 100;
    };

    int poster_id() override {return DUMMY_POSTER_ID;}

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
    bool _received{false};
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
    TestEventDispatcher()
    {
    }

    void SetUp()
    {
        _module_under_test = new EventDispatcher(&_test_engine,
                                                 &_in_rt_queue,
                                                 &_out_rt_queue);
    }

    void TearDown()
    {
        _module_under_test->stop();
        delete _module_under_test;
    }
    EventDispatcher*    _module_under_test;
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

TEST_F(TestEventDispatcher, TestSimpleEventDispatching)
{
    _module_under_test->register_poster(&_poster);
    auto event = new Event(IMMEDIATE_PROCESS);
    event->set_receiver(DUMMY_POSTER_ID);
    _module_under_test->post_event(event);
    crank_event_loop_once();
    ASSERT_TRUE(_poster.event_received());
    _module_under_test->stop();
}

TEST_F(TestEventDispatcher, TestRegisteringAndDeregistering)
{
    auto status = _module_under_test->register_poster(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->register_poster(&_poster);
    EXPECT_EQ(EventDispatcherStatus::ALREADY_SUBSCRIBED, status);

    status = _module_under_test->deregister_poster(&_poster);
    EXPECT_EQ(EventDispatcherStatus::OK, status);
    status = _module_under_test->deregister_poster(&_poster);
    EXPECT_EQ(EventDispatcherStatus::UNKNOWN_POSTER, status);

    status = _module_under_test->subscribe_to_keyboard_events(&_poster);
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
    auto event = new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                 123, 234, IMMEDIATE_PROCESS);
    _module_under_test->post_event(event);

    _module_under_test->subscribe_to_engine_notifications(&_poster);
    crank_event_loop_once();

    ASSERT_TRUE(_poster.event_received());
}


TEST_F(TestEventDispatcher, TestCompletionCallback)
{
    _module_under_test->register_poster(&_poster);
    auto event = new Event(IMMEDIATE_PROCESS);
    event->set_receiver(DUMMY_POSTER_ID);
    event->set_completion_cb(dummy_callback, nullptr);
    completed = false;
    completion_status = 0;

    _module_under_test->post_event(event);
    crank_event_loop_once();

    ASSERT_TRUE(_poster.event_received());
    ASSERT_TRUE(completed);
    ASSERT_EQ(DUMMY_STATUS, completion_status);
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
    EXPECT_EQ(DUMMY_STATUS, typed_event->return_status());
    EXPECT_EQ(sending_ev_id, typed_event->sending_event_id());
    EXPECT_EQ(123u, typed_event->processor_id());
}

class TestWorker : public ::testing::Test
{
public:
    void crank_event_loop_once()
    {
        _module_under_test->_running = false;
        _module_under_test->_worker();
    }
protected:
    TestWorker()
    {
    }

    void SetUp()
    {
        _module_under_test = new Worker(&_test_engine,
                                        _test_engine.event_dispatcher());
    }

    void TearDown()
    {
        _module_under_test->stop();
        delete _module_under_test;
    }
    Worker*          _module_under_test;
    EngineMockup     _test_engine{TEST_SAMPLE_RATE};
};

TEST_F(TestWorker, TestEventQueueingAndProcessing)
{
    completed = false;
    completion_status = 0;
    auto event = new SetEngineTempoEvent(120.0f, IMMEDIATE_PROCESS);
    event->set_completion_cb(dummy_callback, nullptr);
    auto status = _module_under_test->process(event);
    ASSERT_EQ(EventStatus::QUEUED_HANDLING, status);
    ASSERT_FALSE(_module_under_test->_queue.empty());
    crank_event_loop_once();
    ASSERT_TRUE(completed);
    ASSERT_EQ(EventStatus::HANDLED_OK, completion_status);
}
