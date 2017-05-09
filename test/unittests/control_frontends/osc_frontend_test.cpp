
#include <thread>
#include "gtest/gtest.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "control_frontends/base_control_frontend.cpp"
#include "control_frontends/osc_frontend.cpp"
#pragma GCC diagnostic pop

#include "../test_utils.h"

using namespace sushi;
using namespace sushi::control_frontend;

class TestOSCFrontend : public ::testing::Test
{
protected:
    TestOSCFrontend()
    {
    }

    ~TestOSCFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new OSCFrontend(&_event_queue);

        std::stringstream port_stream;
        port_stream << _server_port;
        auto port_str = port_stream.str();
        _address = lo_address_new("localhost", port_str.c_str());
        _module_under_test->run();
    }

    void TearDown()
    {
        lo_address_free(_address);
        delete _module_under_test;
    }

    OSCFrontend* _module_under_test;
    EventFifo _event_queue;
    int _server_port{24024};
    lo_address _address;
};

TEST_F(TestOSCFrontend, test_send_parameter_change_event)
{
    lo_send(_address, "/parameter_change", "ssf", "sampler", "volume", 5.0f);
    // Need to wait a bit to allow messages to come through
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_event_queue.empty());
    Event event;
    ASSERT_TRUE(_event_queue.pop(event));
    EXPECT_EQ(EventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto typed_event = event.parameter_change_event();
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(0u, typed_event->param_id());
    EXPECT_FLOAT_EQ(5.0f, typed_event->value());
}

TEST_F(TestOSCFrontend, test_send_keyboard_event)
{
    lo_send(_address, "/keyboard_event", "ssif", "sampler", "note_on", 46, 0.8f);
    // Need to wait a bit to allow messages to come through
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_event_queue.empty());
    Event event;
    ASSERT_TRUE(_event_queue.pop(event));
    EXPECT_EQ(EventType::NOTE_ON, event.type());
    auto typed_event = event.keyboard_event();
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_FLOAT_EQ(0.8f, typed_event->velocity());
}