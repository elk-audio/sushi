#include <thread>
#include "gtest/gtest.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#define private public
#include "control_frontends/base_control_frontend.cpp"
#include "control_frontends/osc_frontend.cpp"
#pragma GCC diagnostic pop
#include "test_utils/engine_mockup.h"


using namespace sushi;
using namespace sushi::control_frontend;
using namespace sushi::osc;

constexpr float TEST_SAMPLE_RATE = 44100;
constexpr int OSC_TEST_SERVER_PORT = 24024;
constexpr int OSC_TEST_SEND_PORT = 24023;
constexpr int EVENT_WAIT_RETRIES = 20;
constexpr auto EVENT_WAIT_TIME = std::chrono::milliseconds(2);

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
        ASSERT_EQ(ControlFrontendStatus::OK, _module_under_test.init());
        _module_under_test.run();
        _test_dispatcher = static_cast<EventDispatcherMockup*>(_test_engine.event_dispatcher());
    }

    std::unique_ptr<Event> wait_for_event()
    {
        for (int i = 0; i < EVENT_WAIT_RETRIES; ++i)
        {
            auto event = _test_dispatcher->retrieve_event();
            if (event)
            {
                return std::move(event);
            }
            std::this_thread::sleep_for(EVENT_WAIT_TIME);
        }
        return nullptr;
    }

    void TearDown()
    {
        _module_under_test.stop();
        lo_address_free(_address);
    }


    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    int _server_port{OSC_TEST_SERVER_PORT};
    lo_address _address;
    OSCFrontend _module_under_test{&_test_engine, OSC_TEST_SERVER_PORT, OSC_TEST_SEND_PORT};
    EventDispatcherMockup* _test_dispatcher;
};

TEST_F(TestOSCFrontend, TestSendParameterChangeEvent)
{
    ASSERT_TRUE(_module_under_test.connect_to_parameter("sampler", "volume"));
    lo_send(_address, "/parameter/sampler/volume", "f", 5.0f);

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_parameter_change_event());
    auto typed_event = static_cast<ParameterChangeEvent*>(event.get());
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(0u, typed_event->parameter_id());
    EXPECT_EQ(5.0f, typed_event->float_value());
    EXPECT_EQ(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, typed_event->subtype());

    /* Test with a not registered path */
    lo_send(_address, "/parameter/sampler/attack", "f", 5.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestOSCFrontend, TestSendNoteOnEvent)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "note_on", 46, 0.8f);

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_keyboard_event());
    auto typed_event = static_cast<KeyboardEvent*>(event.get());
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_EQ(0.8f, typed_event->velocity());
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, typed_event->subtype());
}

TEST_F(TestOSCFrontend, TestSendNoteOffEvent)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "note_off", 52, 0.7f);

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_keyboard_event());
    auto typed_event = static_cast<KeyboardEvent*>(event.get());
    EXPECT_EQ(0u, typed_event->processor_id());
    EXPECT_EQ(52, typed_event->note());
    EXPECT_EQ(0.7f, typed_event->velocity());
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, typed_event->subtype());

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "sif", "note_off", 46, 0.8f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestOSCFrontend, TestAddTrack)
{
    lo_send(_address, "/engine/add_track", "si", "NewTrack", 2);

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
    auto typed_event = static_cast<AddTrackEvent*>(event.get());
    EXPECT_EQ("NewTrack", typed_event->_name);
    EXPECT_EQ(2, typed_event->_channels);
}

TEST_F(TestOSCFrontend, TestDeleteTrack)
{
    lo_send(_address, "/engine/delete_track", "s", "NewTrack");

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
    auto typed_event = static_cast<RemoveTrackEvent*>(event.get());
    EXPECT_EQ("NewTrack", typed_event->_name);
}

TEST_F(TestOSCFrontend, TestAddProcessor)
{
    lo_send(_address, "/engine/add_processor", "sssss", "track", "uid", "plugin_name", "file_path", "internal");

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
    auto typed_event = static_cast<AddProcessorEvent*>(event.get());
    EXPECT_EQ("track", typed_event->_track);
    EXPECT_EQ("uid", typed_event->_uid);
    EXPECT_EQ("plugin_name", typed_event->_name);
    EXPECT_EQ("file_path", typed_event->_file);
    EXPECT_EQ(AddProcessorEvent::ProcessorType::INTERNAL, typed_event->_processor_type);

    // Test with an invalid processor type, should result in no event being sent
    lo_send(_address, "/engine/add_processor", "sssss", "track", "uid", "plugin_name", "file_path", "ladspa");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestOSCFrontend, TestDeleteProcessor)
{
    lo_send(_address, "/engine/delete_processor", "ss", "track", "processor");

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
    auto typed_event = static_cast<RemoveProcessorEvent*>(event.get());
    EXPECT_EQ("track", typed_event->_track);
    EXPECT_EQ("processor", typed_event->_name);
}

TEST_F(TestOSCFrontend, TestSetTempo)
{
    lo_send(_address, "/engine/set_tempo", "f", 136.0f);

    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
}

TEST_F(TestOSCFrontend, TestSetTimeSignature)
{
    lo_send(_address, "/engine/set_time_signature", "ii", 7, 8);
    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
}

TEST_F(TestOSCFrontend, TestSetPlayingMode)
{
    lo_send(_address, "/engine/set_playing_mode", "s", "playing");
    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
}

TEST_F(TestOSCFrontend, TestSetSyncMode)
{
    lo_send(_address, "/engine/set_sync_mode", "s", "midi");
    auto event = wait_for_event();
    ASSERT_NE(nullptr, event);
    EXPECT_TRUE(event->is_engine_event());
}

TEST(TestOSCFrontendInternal, TestMakeSafePath)
{
    EXPECT_EQ("s_p_a_c_e_", make_safe_path("s p a c e "));
    EXPECT_EQ("in_valid", make_safe_path("in\\\" v*[a]{l}id"));
}