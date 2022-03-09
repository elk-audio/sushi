#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include "control_frontends/base_control_frontend.cpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define private public

#include "test_utils/mock_osc_interface.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"

#include "control_frontends/osc_frontend.cpp"

#undef private
#pragma GCC diagnostic pop

using ::testing::Return;
using ::testing::StrEq;
using ::testing::NiceMock;
using ::testing::_;

using namespace sushi;
using namespace sushi::control_frontend;
using namespace sushi::open_sound_control;

constexpr float TEST_SAMPLE_RATE = 44100;
constexpr int OSC_TEST_SERVER_PORT = 24024;
constexpr int OSC_TEST_SEND_PORT = 24023;
constexpr auto OSC_TEST_SEND_ADDRESS = "127.0.0.1";

class TestOSCFrontend : public ::testing::Test
{
protected:
    TestOSCFrontend() {}

    void SetUp()
    {
        _mock_osc_interface = new MockOscInterface(OSC_TEST_SERVER_PORT, OSC_TEST_SEND_PORT, OSC_TEST_SEND_ADDRESS);

        EXPECT_CALL(*_mock_osc_interface, init()).Times(1).WillOnce(Return(true));

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/set_tempo", "f",
                                                     OscMethodType::SET_TEMPO, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/set_time_signature", "ii",
                                                     OscMethodType::SET_TIME_SIGNATURE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/set_playing_mode", "s",
                                                     OscMethodType::SET_PLAYING_MODE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/set_sync_mode", "s",
                                                     OscMethodType::SET_TEMPO_SYNC_MODE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/set_timing_statistics_enabled", "i",
                                                     OscMethodType::SET_TIMING_STATISTICS_ENABLED, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/reset_timing_statistics", "s",
                                                     OscMethodType::RESET_TIMING_STATISTICS, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method("/engine/reset_timing_statistics", "ss",
                                                     OscMethodType::RESET_TIMING_STATISTICS, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, run()).Times(1).WillOnce(Return(0));

        _module_under_test = std::make_unique<OSCFrontend>(&_mock_engine, &_mock_controller, _mock_osc_interface);

        ASSERT_EQ(ControlFrontendStatus::OK, _module_under_test->init());

        _module_under_test->run();
    }

    void TearDown()
    {
        EXPECT_CALL(*_mock_osc_interface, stop()).Times(1).WillOnce(Return(0));

        EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(7);

        _module_under_test->stop();
    }

    MockOscInterface* _mock_osc_interface {nullptr};

    EngineMockup _mock_engine {TEST_SAMPLE_RATE};
    sushi::ext::ControlMockup _mock_controller;

    std::unique_ptr<OSCFrontend> _module_under_test;
};

TEST_F(TestOSCFrontend, TestFailedInit)
{
    EXPECT_CALL(*_mock_osc_interface, init()).Times(1).WillOnce(Return(false));
    ASSERT_EQ(ControlFrontendStatus::INTERFACE_UNAVAILABLE, _module_under_test->init());
}

TEST_F(TestOSCFrontend, TestConnectAll)
{
    // Track 1
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/track_1/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);

    // The below are called twice, Because we are using a mock engine,
    // which returns the same processors for both tracks, meaning also the processor names are the same.
    // This model is not really allowed - it wouldn't load correctly.
    // ProcessorContainerMockup would need changing to make it consistent with a "Real File".

    // Proc 1
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/proc_1/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/proc_1"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(2);

    // Proc 2
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_2/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_2/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_2/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/proc_2/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/program/proc_2"), "i",
                                                 OscMethodType::SEND_PROGRAM_CHANGE_EVENT, _)).Times(2);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/proc_2"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(2);

    // Keyboard track 1
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    // Track 2
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_2/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_2/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_2/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/track_2/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);

    // Keyboard track 2
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_2"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_2"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    _module_under_test->connect_to_all();
}

TEST_F(TestOSCFrontend, TestConnectFromAllParameters)
{
    auto enabled_outputs_empty = _module_under_test->get_enabled_parameter_outputs();
    EXPECT_EQ(enabled_outputs_empty.size(), 0);

    _module_under_test->connect_from_all_parameters();

    auto enabled_outputs_full = _module_under_test->get_enabled_parameter_outputs();

    EXPECT_EQ(enabled_outputs_full.size(), 1);

    _module_under_test->disconnect_from_all_parameters();

    enabled_outputs_empty = _module_under_test->get_enabled_parameter_outputs();
    EXPECT_EQ(enabled_outputs_empty.size(), 0);
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnectionsForProcessor)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/proc_1"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/program/proc_1"), "i",
                                                 OscMethodType::SEND_PROGRAM_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc_1/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/proc_1/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);

    // As this in only done in response to events, test the event handling at the same time
    ObjectId processor_id = 0;

    auto event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_CREATED,
                                             processor_id, 0, IMMEDIATE_PROCESS);
    _module_under_test->process(&event);

    EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(6);

    event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_DELETED,
                                        processor_id, 0, IMMEDIATE_PROCESS);

    _module_under_test->process(&event);
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnectionsForTrack)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/track_1"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/track_1/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);


    // As this in only done in response to events, test the event handling at the same time
    ObjectId track_id = 0;

    auto event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_CREATED, track_id, 0, IMMEDIATE_PROCESS);
    _module_under_test->process(&event);

    EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(7);

    event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_DELETED, track_id, 0, IMMEDIATE_PROCESS);

    _module_under_test->process(&event);
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnectionsForTrack)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track_1"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/track_1"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_1"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_2"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track_1/param_3"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/track_1/property_1"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);


    // As this in only done in response to events, test the event handling at the same time
    ObjectId track_id = 0;

    auto event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_CREATED, track_id, 0, IMMEDIATE_PROCESS);
    _module_under_test->process(&event);

    EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(7);

    event = AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_DELETED, track_id, 0, IMMEDIATE_PROCESS);

    _module_under_test->process(&event);
}

TEST_F(TestOSCFrontend, TestSendParameterChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/sampler/volume"), "f",
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    auto connection = _module_under_test->_connect_to_parameter("sampler", "volume", 0, 0);

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.parameter_controller();
    controller->set_parameter_value(connection->processor, connection->parameter, 5.0f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.parameter_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(0, std::stoi(args["parameter id"]));
    EXPECT_FLOAT_EQ(5.0f, std::stof(args["value"]));
}

TEST_F(TestOSCFrontend, TestSendPropertyChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/sampler/sample_file"), "s",
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);

    auto connection = _module_under_test->_connect_to_property("sampler", "sample_file", 0, 0);

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.parameter_controller();
    controller->set_property_value(connection->processor, connection->parameter, "Sample file");

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.parameter_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(0, std::stoi(args["property id"]));
    EXPECT_EQ("Sample file", args["value"]);
}

TEST_F(TestOSCFrontend, TestSendNoteOn)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_note_on(connection->processor, 0, 46, 0.8f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(0, std::stoi(args["channel"]));
    EXPECT_EQ(46, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.8f, std::stof(args["velocity"]));
}

TEST_F(TestOSCFrontend, TestSendNoteOff)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_note_off(connection->processor, 1, 52, 0.7f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(1, std::stoi(args["channel"]));
    EXPECT_EQ(52, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.7f, std::stof(args["velocity"]));
}

TEST_F(TestOSCFrontend, TestSendNoteAftertouch)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_note_aftertouch(connection->processor, 10, 36, 0.1f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(10, std::stoi(args["channel"]));
    EXPECT_EQ(36, std::stoi(args["note"]));
    EXPECT_FLOAT_EQ(0.1f, std::stof(args["value"]));
}

TEST_F(TestOSCFrontend, TestSendKeyboardModulation)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_modulation(connection->processor, 9, 0.5f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(9, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.5f, std::stof(args["value"]));
}

TEST_F(TestOSCFrontend, TestSendKeyboardPitchBend)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_pitch_bend(connection->processor, 3, 0.3f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(3, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.3f, std::stof(args["value"]));
}

TEST_F(TestOSCFrontend, TestSendKeyboardAftertouch)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "siif",
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/sampler"), "sif",
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_kb_to_track("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.keyboard_controller();
    controller->send_aftertouch(connection->processor, 11, 0.11f);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.keyboard_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["track id"]));
    EXPECT_EQ(11, std::stoi(args["channel"]));
    EXPECT_FLOAT_EQ(0.11f, std::stof(args["value"]));
}

TEST_F(TestOSCFrontend, TestSendProgramChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/program/sampler"), "i",
                                                 OscMethodType::SEND_PROGRAM_CHANGE_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_to_program_change("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.program_controller();
    controller->set_processor_program(connection->processor, 1);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.program_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ(1, std::stoi(args["program id"]));
}

TEST_F(TestOSCFrontend, TestSetBypassState)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/sampler"), "i",
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    auto connection = _module_under_test->connect_to_bypass_state("sampler");

    ASSERT_TRUE(connection != nullptr);

    auto controller = _mock_controller.audio_graph_controller_mockup();
    controller->set_processor_bypass_state(connection->processor, 1);

    ASSERT_TRUE(_mock_controller.was_recently_called());

    auto args = _mock_controller.audio_graph_controller_mockup()->get_args_from_last_call();
    EXPECT_EQ(0, std::stoi(args["processor id"]));
    EXPECT_EQ("1", args["bypass enabled"]);
}

TEST_F(TestOSCFrontend, TestParamChangeNotification)
{
    EXPECT_CALL(*_mock_osc_interface, send(StrEq("/parameter/proc_2/param_3"), testing::Matcher<float>(0.5f))).Times(1);

    ObjectId processor_id = 0;
    ObjectId parameter_id = 0;

    auto event = ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT,
                                                  processor_id,
                                                  parameter_id,
                                                  0.5f,
                                                  IMMEDIATE_PROCESS);

    _module_under_test->process(&event); // Since nothing is connected this should not cause a call.

    _module_under_test->connect_from_all_parameters();

    _module_under_test->process(&event); // But this should - the one expected.
}

TEST(TestOSCFrontendInternal, TestMakeSafePath)
{
    EXPECT_EQ("s_p_a_c_e_", make_safe_path("s p a c e "));
    EXPECT_EQ("in_valid", make_safe_path("in\\\" v*[a]{l}id"));
}
