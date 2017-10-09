#include <thread>
#include "gtest/gtest.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "control_frontends/base_control_frontend.cpp"
#include "control_frontends/osc_frontend.cpp"
#pragma GCC diagnostic pop

#include "audio_frontends/engine_mockup.h"

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
        std::stringstream port_stream;
        port_stream << _server_port;
        auto port_str = port_stream.str();
        _address = lo_address_new("localhost", port_str.c_str());
        _module_under_test.run();
    }

    void TearDown()
    {
        lo_address_free(_address);
    }
    EngineMockup _test_engine{44100};
    EventFifo _event_queue;
    int _server_port{24024};
    lo_address _address;
    OSCFrontend _module_under_test{&_event_queue, &_test_engine};
};


TEST_F(TestOSCFrontend, TestSendParameterChangeEvent)
{
    ASSERT_TRUE(_module_under_test.connect_to_parameter("sampler", "volume"));
    lo_send(_address, "/parameter/sampler/volume", "f", 5.0f);

    // Need to wait a bit to allow messages to come through
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_event_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_event_queue.pop(event));
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto typed_event = event.parameter_change_event();
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(0u, typed_event->param_id());
    EXPECT_FLOAT_EQ(5.0f, typed_event->value());

    /* Test with a not registered path */
    lo_send(_address, "/parameter/sampler/attack", "f", 5.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_TRUE(_event_queue.empty());
}

TEST_F(TestOSCFrontend, TestSendKeyboardEvent)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "note_on", 46, 0.8f);

    // Need to wait a bit to allow messages to come through
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_event_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_event_queue.pop(event));
    EXPECT_EQ(RtEventType::NOTE_ON, event.type());
    auto typed_event = event.keyboard_event();
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_FLOAT_EQ(0.8f, typed_event->velocity());

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "sif", "note_off", 46, 0.8f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_TRUE(_event_queue.empty());
}

TEST(TestOSCFrontendInternal, TestSpacesToUnderscores)
{
    std::string test_str("str with spaces ");
    ASSERT_EQ("str_with_spaces_", spaces_to_underscore(test_str));
}