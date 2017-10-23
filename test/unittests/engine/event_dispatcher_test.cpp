#include "gtest/gtest.h"
#include "test_utils.h"

#include "engine/event_dispatcher.cpp"
#include "../audio_frontends/engine_mockup.h"

using namespace sushi;
using namespace sushi::dispatcher;


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
        delete _module_under_test;
    }
    EventDispatcher* _module_under_test;
    EngineMockup     _test_engine{44100};
    RtEventFifo      _in_rt_queue;
    RtEventFifo      _out_rt_queue;
};

TEST_F(TestEventDispatcher, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    _module_under_test->run();
    Event e;
    std::cout << "Event size: " << sizeof(e) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    _module_under_test->stop();
}