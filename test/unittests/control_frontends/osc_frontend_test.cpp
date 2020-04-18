#include <thread>
#include "gtest/gtest.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#define private public
#include "control_frontends/base_control_frontend.cpp"
#include "control_frontends/osc_frontend.cpp"
#pragma GCC diagnostic pop
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"


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
    TestOSCFrontend() {}

    void SetUp()
    {
        std::stringstream port_stream;
        port_stream << _server_port;
        auto port_str = port_stream.str();
        _address = lo_address_new("localhost", port_str.c_str());
        ASSERT_EQ(ControlFrontendStatus::OK, _module_under_test.init());
        _module_under_test.run();
    }

    // If the test expects the events NOT to be received, set the number of
    // retries to something low (1-2) to keep test execution times down
    bool wait_for_event(int retries = EVENT_WAIT_RETRIES)
    {
        for (int i = 0; i < retries; ++i)
        {
            auto event = _controller.was_recently_called();
            if (event)
            {
                _controller.clear_recent_call();
                return event;
            }
            std::this_thread::sleep_for(EVENT_WAIT_TIME);
        }
        return false;
    }

    void TearDown()
    {
        _module_under_test.stop();
        lo_address_free(_address);
    }

    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    int _server_port{OSC_TEST_SERVER_PORT};
    lo_address _address;
    sushi::ext::ControlMockup _controller;
    OSCFrontend _module_under_test{&_test_engine, &_controller, OSC_TEST_SERVER_PORT, OSC_TEST_SEND_PORT};
};

TEST_F(TestOSCFrontend, TestConnectAll)
{
    _module_under_test.connect_all();
    lo_send(_address, "/parameter/track_1/param_1", "f", 0.5f);
    EXPECT_TRUE(wait_for_event());
    lo_send(_address, "/parameter/track_2/param_2", "f", 0.5f);
    EXPECT_TRUE(wait_for_event());
    lo_send(_address, "/parameter/proc_1/param_1", "f", 0.5f);
    EXPECT_TRUE(wait_for_event());
    lo_send(_address, "/parameter/proc_2/param_2", "f", 0.5f);
    EXPECT_TRUE(wait_for_event());
    lo_send(_address, "/parameter/non/existing", "f", 0.5f);
    ASSERT_FALSE(wait_for_event(2));
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnections)
{
    // As this in only done in response to events, test the event handling at the same time
    ObjectId processor_id = 0;
    auto event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Subtype::PROCESSOR_ADDED,
                                             processor_id, 0, IMMEDIATE_PROCESS);
    _module_under_test.process(&event);
    lo_send(_address, "/parameter/proc_1/param_1", "f", 0.5f);
    EXPECT_TRUE(wait_for_event());

    event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Subtype::PROCESSOR_REMOVED,
                                        processor_id, 0, IMMEDIATE_PROCESS);

    _module_under_test.process(&event);
    lo_send(_address, "/parameter/proc_1/param_1", "f", 0.5f);
    EXPECT_FALSE(wait_for_event(2));
}

TEST_F(TestOSCFrontend, TestSendParameterChange)
{
    ASSERT_TRUE(_module_under_test.connect_to_parameter("sampler", "volume"));
    lo_send(_address, "/parameter/sampler/volume", "f", 5.0f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(0, std::stoi(args["parameter id"]));
    EXPECT_FLOAT_EQ(5.0f, std::stof(args["value"]));

    /* Test with a not registered path */
    lo_send(_address, "/parameter/sampler/attack", "f", 5.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendNoteOn)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "siif", "note_on", 0, 46, 0.8f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(0, std::stoi(args["channel"]));
    EXPECT_EQ(46, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.8f, std::stof(args["velocity"]));

     // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "siif", "note_off", 2, 37, 0.8f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendNoteOff)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "siif", "note_off", 1, 52, 0.7f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(1, std::stoi(args["channel"]));
    EXPECT_EQ(52, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.7f, std::stof(args["velocity"]));

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "siif", "note_off", 3, 46, 0.8f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendNoteAftertouch)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "siif", "note_aftertouch", 10, 36, 0.1f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(10, std::stoi(args["channel"]));
    EXPECT_EQ(36, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.1f, std::stof(args["value"]));

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "siif", "note_aftertouch", 4, 20, 0.2f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendKeyboardModulation)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "modulation", 9, 0.5f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(9, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.5f, std::stof(args["value"]));

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "sif", "modulation", 4, 0.2f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendKeyboardPitchBend)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "pitch_bend", 3, 0.3f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(3, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.3f, std::stof(args["value"]));

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "sif", "pitch_bend", 1, 0.2f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendKeyboardAftertouch)
{
    ASSERT_TRUE(_module_under_test.connect_kb_to_track("sampler"));
    lo_send(_address, "/keyboard_event/sampler", "sif", "aftertouch", 11, 0.11f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(11, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.11f, std::stof(args["value"]));

    // Test with a path not registered
    lo_send(_address, "/keyboard_event/drums", "sif", "aftertouch", 12, 0.52f);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSendProgramChange)
{
    ASSERT_TRUE(_module_under_test.connect_to_program_change("sampler"));
    lo_send(_address, "/program/sampler", "i", 1);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(1, std::stoi(args["program id"]));

    // Test with a path not registerd
    lo_send(_address, "program/drums", "i", 2);
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSetBypassState)
{
    ASSERT_TRUE(_module_under_test.connect_to_bypass_state("sampler"));
    lo_send(_address, "/bypass/sampler", "i", 1);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ("1", args["bypass enabled"]);

    // Test with a path not registered
    lo_send(_address, "bypass/drums", "i", 0);
    ASSERT_FALSE(_controller.was_recently_called());
}

TEST_F(TestOSCFrontend, TestSetTempo)
{
    lo_send(_address, "/engine/set_tempo", "f", 136.0f);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_FLOAT_EQ(136.0, std::stof(args["tempo"]));
}

TEST_F(TestOSCFrontend, TestSetTimeSignature)
{
    lo_send(_address, "/engine/set_time_signature", "ii", 7, 8);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(7, std::stoi(args["numerator"]));
    EXPECT_EQ(8, std::stoi(args["denominator"]));
}

TEST_F(TestOSCFrontend, TestSetPlayingMode)
{
    lo_send(_address, "/engine/set_playing_mode", "s", "playing");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ("PLAYING", args["playing mode"]);
}

TEST_F(TestOSCFrontend, TestSetSyncMode)
{
    lo_send(_address, "/engine/set_sync_mode", "s", "midi");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ("MIDI", args["sync mode"]);
}

TEST_F(TestOSCFrontend, TestSetTimingStatisticsEnabled)
{
    lo_send(_address, "/engine/set_timing_statistics_enabled", "i", 1);

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ("1", args["enabled"]);
}

TEST_F(TestOSCFrontend, TestResetAllTimings)
{
    lo_send(_address, "/engine/reset_timing_statistics", "s", "all");

    ASSERT_TRUE(wait_for_event());
}

TEST_F(TestOSCFrontend, TestResetTrackTimings)
{
    lo_send(_address, "/engine/reset_timing_statistics", "ss", "track", "main");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
}

TEST_F(TestOSCFrontend, TestResetProcessorTimings)
{
    lo_send(_address, "/engine/reset_timing_statistics", "ss", "processor", "sampler");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
}

TEST_F(TestOSCFrontend, TestAddProcessorToTrack)
{
    lo_send(_address, "/engine/add_processor_to_track", "sssss", "plugin", "", "lib.so", "vst2x", "track");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ("plugin", args["name"]);
    EXPECT_EQ("", args["uid"]);
    EXPECT_EQ("lib.so", args["file"]);
    EXPECT_EQ(static_cast<int>(PluginType::VST2X), std::stoi(args["type"]));
    EXPECT_EQ(0, std::stoi(args["track_id"]));
    EXPECT_EQ(-1, std::stoi(args["before_processor_id"]));

    lo_send(_address, "/engine/add_processor_to_track", "ssssss", "plugin", "uid", "lib.so", "vst3x", "track", "track");

    ASSERT_TRUE(wait_for_event());
    args = _controller.get_args_from_last_call();
    EXPECT_EQ("plugin", args["name"]);
    EXPECT_EQ("uid", args["uid"]);
    EXPECT_EQ("lib.so", args["file"]);
    EXPECT_EQ(static_cast<int>(PluginType::VST3X), std::stoi(args["type"]));
    EXPECT_EQ(0, std::stoi(args["track_id"]));
    EXPECT_EQ(0, std::stoi(args["before_processor_id"]));
}

TEST_F(TestOSCFrontend, TestMoveProcessorOnTrack)
{
    lo_send(_address, "/engine/move_processor_on_track", "sss", "plugin", "track", "track_1");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor_id"]));
    EXPECT_EQ(0, std::stoi(args["source_track_id"]));
    EXPECT_EQ(0, std::stoi(args["dest_track_id"]));
    EXPECT_EQ(-1, std::stoi(args["before_processor_id"]));

    lo_send(_address, "/engine/move_processor_on_track", "ssss", "plugin", "track", "track_1", "track_2");

    ASSERT_TRUE(wait_for_event());
    args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor_id"]));
    EXPECT_EQ(0, std::stoi(args["source_track_id"]));
    EXPECT_EQ(0, std::stoi(args["dest_track_id"]));
    EXPECT_EQ(0, std::stoi(args["before_processor_id"]));
}

TEST_F(TestOSCFrontend, TestDeleteProcesorFromTrack)
{
    lo_send(_address, "/engine/delete_processor_from_track", "ss", "plugin", "track");

    ASSERT_TRUE(wait_for_event());
    auto args = _controller.get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor_id"]));
    EXPECT_EQ(0, std::stoi(args["track_id"]));
}

TEST(TestOSCFrontendInternal, TestMakeSafePath)
{
    EXPECT_EQ("s_p_a_c_e_", make_safe_path("s p a c e "));
    EXPECT_EQ("in_valid", make_safe_path("in\\\" v*[a]{l}id"));
}
