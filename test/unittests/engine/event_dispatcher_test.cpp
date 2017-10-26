#include "gtest/gtest.h"
#include "test_utils.h"

#define private public
#define protected public

#include "engine/event_dispatcher.cpp"
#include "../audio_frontends/engine_mockup.h"

using namespace sushi;
using namespace sushi::dispatcher;

constexpr int DUMMY_POSTER_ID = 1;
constexpr int DUMMY_STATUS = 100;
constexpr auto EVENT_PROCESS_WAIT_TIME = std::chrono::microseconds(20);

bool completed = false;
int completion_status = 0;

void dummy_callback(void* /*arg*/, Event* /*event*/, int status)
{
    completed = true;
    completion_status = status;
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

    const std::string& poster_name() {return _name;}

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
    std::string _name{"Dummy"};
};

class TestEventDispatcher : public ::testing::Test
{
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
    EventDispatcher* _module_under_test;
    EngineMockup     _test_engine{44100};
    RtEventFifo      _in_rt_queue;
    RtEventFifo      _out_rt_queue;

    DummyPoster      _poster;
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
    _module_under_test->run();
    auto event = new Event(EventType::BASIC_EVENT, 0);
    event->set_receiver(DUMMY_POSTER_ID);
    _module_under_test->post_event(event);
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);

    ASSERT_TRUE(_poster.event_received());
}

TEST_F(TestEventDispatcher, TestToRtEvent)
{
    auto event = new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, "processor", 50, 1.0f, 0);
    _module_under_test->post_event(event);
    ASSERT_TRUE(_out_rt_queue.empty());
    _module_under_test->run();
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);

    ASSERT_FALSE(_out_rt_queue.empty());
    RtEvent rt_event;
    _out_rt_queue.pop(rt_event);
    ASSERT_EQ(RtEventType::NOTE_ON, rt_event.type());
}

TEST_F(TestEventDispatcher, TestFromRtEventNoteOnEvent)
{
    RtEvent rt_event = RtEvent::make_note_on_event(10, 0, 50, 10.f);
    _in_rt_queue.push(rt_event);

    _module_under_test->subscribe_to_keyboard_events(&_poster);
    _module_under_test->run();
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);

    ASSERT_TRUE(_poster.event_received());
}

TEST_F(TestEventDispatcher, TestFromRtEventParameterChangeNotification)
{
    RtEvent rt_event = RtEvent::make_parameter_change_event(10, 0, 10, 5.f);
    _in_rt_queue.push(rt_event);

    _module_under_test->subscribe_to_parameter_change_notifications(&_poster);
    _module_under_test->run();
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);

    ASSERT_TRUE(_poster.event_received());
}


TEST_F(TestEventDispatcher, TestCompletionCallback)
{
    _module_under_test->register_poster(&_poster);
    _module_under_test->run();
    auto event = new Event(EventType::BASIC_EVENT, 0);
    event->set_receiver(DUMMY_POSTER_ID);
    event->set_completion_cb(dummy_callback, nullptr);
    completed = false;
    completion_status = 0;

    _module_under_test->post_event(event);
    std::this_thread::sleep_for(EVENT_PROCESS_WAIT_TIME);

    ASSERT_TRUE(_poster.event_received());
    ASSERT_TRUE(completed);
    ASSERT_EQ(DUMMY_STATUS, completion_status);

}
