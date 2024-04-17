#include <thread>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include "control_frontends/base_control_frontend.cpp"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_UNUSED_PARAMETER

#include "test_utils/mock_osc_interface.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"
#include "test_utils/mock_processor_container.h"
#include "test_utils/host_control_mockup.h"

#include "control_frontends/osc_frontend.cpp"

ELK_POP_WARNING

using ::testing::Return;
using ::testing::StrEq;
using ::testing::NiceMock;
using ::testing::_;

namespace sushi::internal::control_frontend
{

class OSCFrontendAccessor
{
public:
    explicit OSCFrontendAccessor(OSCFrontend& f) : _friend(f) {}

    void set_processor_container(const engine::BaseProcessorContainer* container)
    {
        _friend._processor_container = container;
    }

    OscConnection* connect_to_parameter(const std::string& processor_name,
                                        const std::string& parameter_name,
                                        ObjectId processor_id,
                                        ObjectId parameter_id)
    {
        return _friend._connect_to_parameter(processor_name, parameter_name, processor_id, parameter_id);
    }

    OscConnection* connect_to_property(const std::string& processor_name,
                                       const std::string& property_name,
                                       ObjectId processor_id,
                                       ObjectId property_id)
    {
        return _friend._connect_to_property(processor_name, property_name, processor_id, property_id);
    }

    OscConnection* connect_kb_to_track(const Processor* processor)
    {
        return _friend._connect_kb_to_track(processor);
    }

    OscConnection* connect_to_program_change(const Processor* processor)
    {
        return _friend._connect_to_program_change(processor);
    }

    OscConnection* connect_to_bypass_state(const Processor* processor)
    {
        return _friend._connect_to_bypass_state(processor);
    }

private:
    OSCFrontend& _friend;
};

}

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::control_frontend;
using namespace sushi::internal::osc;

constexpr float TEST_SAMPLE_RATE = 44100;
constexpr int OSC_TEST_SERVER_PORT = 24024;
constexpr int OSC_TEST_SEND_PORT = 24023;
constexpr auto OSC_TEST_SEND_ADDRESS = "127.0.0.1";
constexpr auto TEST_TRACK_NAME = "track";
constexpr auto TEST_PROCESSOR_NAME = "proc";

class TestOSCFrontend : public ::testing::Test
{
protected:
    TestOSCFrontend() = default;

    void SetUp() override
    {
        _mock_osc_interface = new MockOscInterface(OSC_TEST_SERVER_PORT, OSC_TEST_SEND_PORT, OSC_TEST_SEND_ADDRESS);

        EXPECT_CALL(*_mock_osc_interface, init()).Times(1).WillOnce(Return(true));

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/set_tempo"), StrEq("f"),
                                                     OscMethodType::SET_TEMPO, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/set_time_signature"), StrEq("ii"),
                                                     OscMethodType::SET_TIME_SIGNATURE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/set_playing_mode"), StrEq("s"),
                                                     OscMethodType::SET_PLAYING_MODE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/set_sync_mode"), StrEq("s"),
                                                     OscMethodType::SET_TEMPO_SYNC_MODE, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/set_timing_statistics_enabled"), StrEq("i"),
                                                     OscMethodType::SET_TIMING_STATISTICS_ENABLED, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/reset_timing_statistics"), StrEq("s"),
                                                     OscMethodType::RESET_TIMING_STATISTICS, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/engine/reset_timing_statistics"), StrEq("ss"),
                                                     OscMethodType::RESET_TIMING_STATISTICS, _)).Times(1);

        EXPECT_CALL(*_mock_osc_interface, run()).Times(1);

        _module_under_test = std::make_unique<OSCFrontend>(&_mock_engine, &_mock_controller, _mock_osc_interface);
        _test_processor = std::make_shared<DummyProcessor>(_host_control_mockup.make_host_control_mockup());
        _test_track = std::make_shared<Track>(_host_control_mockup.make_host_control_mockup(), 2, nullptr, true);
        _test_track->set_name(TEST_TRACK_NAME);
        _test_processor->set_name(TEST_PROCESSOR_NAME);

        ASSERT_EQ(ControlFrontendStatus::OK, _module_under_test->init());

        _accessor = std::make_unique<OSCFrontendAccessor>(*_module_under_test);

        // Inject the mock container
        _accessor->set_processor_container(&_mock_processor_container);
        _module_under_test->run();

        // Set up default returns for mock processor container
        ON_CALL(_mock_processor_container, all_processors()).WillByDefault(Return(std::vector<std::shared_ptr<const Processor>>({_test_track, _test_processor})));
        ON_CALL(_mock_processor_container, all_tracks()).WillByDefault(Return(std::vector<std::shared_ptr<const Track>>({_test_track})));
        ON_CALL(_mock_processor_container, processors_on_track(_test_track->id())).WillByDefault(Return(std::vector<std::shared_ptr<const Processor>>({_test_processor})));
        ON_CALL(_mock_processor_container, track(::testing::A<const std::string&>())).WillByDefault(Return(_test_track));
        ON_CALL(_mock_processor_container, track(::testing::A<ObjectId>())).WillByDefault(Return(_test_track));
        ON_CALL(_mock_processor_container, processor(_test_track->name())).WillByDefault(Return(_test_track));
        ON_CALL(_mock_processor_container, processor(_test_track->id())).WillByDefault(Return(_test_track));
        ON_CALL(_mock_processor_container, processor(_test_processor->name())).WillByDefault(Return(_test_processor));
        ON_CALL(_mock_processor_container, processor(_test_processor->id())).WillByDefault(Return(_test_processor));
    }

    void TearDown() override
    {
        EXPECT_CALL(*_mock_osc_interface, stop()).Times(1);

        EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(7);

        _module_under_test->stop();
    }

    ::testing::NiceMock<MockProcessorContainer>  _mock_processor_container;
    MockOscInterface* _mock_osc_interface {nullptr};

    EngineMockup _mock_engine {TEST_SAMPLE_RATE};
    sushi::control::ControlMockup _mock_controller;

    std::unique_ptr<OSCFrontend> _module_under_test;
    std::unique_ptr<OSCFrontendAccessor> _accessor;

    HostControlMockup _host_control_mockup;
    std::shared_ptr<Processor> _test_processor;
    std::shared_ptr<Track> _test_track;
};

TEST_F(TestOSCFrontend, TestFailedInit)
{
    EXPECT_CALL(*_mock_osc_interface, init()).Times(1).WillOnce(Return(false));
    ASSERT_EQ(ControlFrontendStatus::INTERFACE_UNAVAILABLE, _module_under_test->init());
}

TEST_F(TestOSCFrontend, TestConnectFromAllParameters)
{
    auto enabled_outputs_empty = _module_under_test->get_enabled_parameter_outputs();
    EXPECT_EQ(enabled_outputs_empty.size(), 0);

    _module_under_test->connect_from_all_parameters();

    auto enabled_outputs_full = _module_under_test->get_enabled_parameter_outputs();

    EXPECT_EQ(enabled_outputs_full.size(), 5);

    _module_under_test->disconnect_from_all_parameters();

    enabled_outputs_empty = _module_under_test->get_enabled_parameter_outputs();
    EXPECT_EQ(enabled_outputs_empty.size(), 0);
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnectionsForProcessor)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/proc"), StrEq("i"),
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/program/proc"), StrEq("i"),
                                                 OscMethodType::SEND_PROGRAM_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc/param_1"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc/gain"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    // As this in only done in response to events, test the event handling at the same time
    ObjectId processor_id = _test_processor->id();

    auto event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_CREATED,
                                                               processor_id, 0, IMMEDIATE_PROCESS);
    _module_under_test->process(event.get());

    EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(4);

    event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::PROCESSOR_DELETED,
                                                          processor_id, 0, IMMEDIATE_PROCESS);

    _module_under_test->process(event.get());
}

TEST_F(TestOSCFrontend, TestAddAndRemoveConnectionsForTrack)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track"), StrEq("siif"),
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track"), StrEq("sif"),
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/track"), StrEq("i"),
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track/gain"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track/pan"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/track/mute"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    // As this in only done in response to events, test the event handling at the same time
    ObjectId track_id = _test_track->id();

    auto event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::TRACK_CREATED,
                                                               track_id, 0, IMMEDIATE_PROCESS);
    _module_under_test->process(event.get());

    EXPECT_CALL(*_mock_osc_interface, delete_method(_)).Times(6);

    event = std::make_unique<AudioGraphNotificationEvent>(AudioGraphNotificationEvent::Action::TRACK_DELETED,
                                                          0, track_id, IMMEDIATE_PROCESS);

    _module_under_test->process(event.get());
}

TEST_F(TestOSCFrontend, TestConnectParameterChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/parameter/proc/param_1"), StrEq("f"),
                                                 OscMethodType::SEND_PARAMETER_CHANGE_EVENT, _)).Times(1);

    auto connection = _accessor->connect_to_parameter("proc", "param 1", 1, 2);
    ASSERT_TRUE(connection != nullptr);
    EXPECT_EQ(1, connection->processor);
    EXPECT_EQ(2, connection->parameter);
}

TEST_F(TestOSCFrontend, TestConnectPropertyChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/property/sampler/sample_file"), StrEq("s"),
                                                 OscMethodType::SEND_PROPERTY_CHANGE_EVENT, _)).Times(1);

    auto connection = _accessor->connect_to_property("sampler", "sample_file", 1, 2);
    ASSERT_TRUE(connection != nullptr);
    EXPECT_EQ(1, connection->processor);
    EXPECT_EQ(2, connection->parameter);
}

TEST_F(TestOSCFrontend, TestAddKbdToTrack)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track"), StrEq("siif"),
                                                 OscMethodType::SEND_KEYBOARD_NOTE_EVENT, _)).Times(1);

    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/keyboard_event/track"), StrEq("sif"),
                                                 OscMethodType::SEND_KEYBOARD_MODULATION_EVENT, _)).Times(1);

    auto connection = _accessor->connect_kb_to_track(_test_track.get());
    EXPECT_EQ(_test_track->id(), connection->processor);

    ASSERT_TRUE(connection != nullptr);
}

TEST_F(TestOSCFrontend, TestConnectProgramChange)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/program/proc"), StrEq("i"),
                                                 OscMethodType::SEND_PROGRAM_CHANGE_EVENT, _)).Times(1);

    auto connection = _accessor->connect_to_program_change(_test_processor.get());
    ASSERT_TRUE(connection != nullptr);
    EXPECT_EQ(_test_processor->id(), connection->processor);
}

TEST_F(TestOSCFrontend, TestSetBypassState)
{
    EXPECT_CALL(*_mock_osc_interface, add_method(StrEq("/bypass/proc"), StrEq("i"),
                                                 OscMethodType::SEND_BYPASS_STATE_EVENT, _)).Times(1);

    auto connection = _accessor->connect_to_bypass_state(_test_processor.get());

    ASSERT_TRUE(connection != nullptr);
    EXPECT_EQ(_test_processor->id(), connection->processor);
}

TEST_F(TestOSCFrontend, TestParamChangeNotification)
{
    EXPECT_CALL(*_mock_osc_interface, send(StrEq("/parameter/proc/param_1"), testing::Matcher<float>(0.5f))).Times(1);

    ObjectId processor_id = _test_processor->id();
    ObjectId parameter_id = _test_processor->parameter_from_name("param 1")->id();

    auto event = std::make_unique<ParameterChangeNotificationEvent>(processor_id,
                                                                    parameter_id,
                                                                    0.5f,
                                                                    0.0f,
                                                                    "",
                                                                    IMMEDIATE_PROCESS);

    _module_under_test->process(event.get()); // Since nothing is connected this should not cause a call.

    _module_under_test->connect_from_all_parameters();

    event = std::make_unique<ParameterChangeNotificationEvent>(processor_id,
                                                               parameter_id,
                                                               0.5f,
                                                               0.0f,
                                                               "",
                                                               IMMEDIATE_PROCESS);

    _module_under_test->process(event.get()); // But this should - the one expected.
}

/*TEST_F(TestOSCFrontend, TestPropertyChangeNotification)
{
    EXPECT_CALL(*_mock_osc_interface, send(StrEq("/parameter/proc/property_1"), testing::Matcher<float>(0.5f))).Times(1);

    ObjectId processor_id = _test_processor->id();
    ObjectId parameter_id = _test_processor->parameter_from_name("param 1")->id();

    auto event = ParameterChangeNotificationEvent(ParameterChangeNotificationEvent::Subtype::FLOAT_PARAMETER_CHANGE_NOT,
                                                  processor_id,
                                                  parameter_id,
                                                  0.5f,
                                                  IMMEDIATE_PROCESS);

    _module_under_test->process(&event); // Since nothing is connected this should not cause a call.

    _module_under_test->connect_from_all_parameters();

    _module_under_test->process(&event); // But this should - the one expected.
}*/

TEST_F(TestOSCFrontend, TestStateHandling)
{
    _module_under_test->set_connect_from_all_parameters(true);
    _module_under_test->connect_from_all_parameters();

    auto state = _module_under_test->save_state();

    EXPECT_TRUE(state.auto_enable_outputs());

    auto outputs = state.enabled_outputs();
    ASSERT_EQ(2, outputs.size());
    ASSERT_EQ(TEST_PROCESSOR_NAME, outputs.front().first);
    auto params = outputs.front().second;
    ASSERT_EQ(2, params.size());
    EXPECT_EQ(0, params.front());

    _module_under_test->disconnect_from_all_parameters();
    ASSERT_EQ(0, _module_under_test->get_enabled_parameter_outputs().size());

    _module_under_test->set_state(state);
    auto output_paths = _module_under_test->get_enabled_parameter_outputs();
    ASSERT_EQ(5, output_paths.size());
    EXPECT_EQ("/parameter/proc/param_1", output_paths.front());
}

TEST(TestOSCFrontendInternal, TestMakeSafePath)
{
    EXPECT_EQ("s_p_a_c_e_", make_safe_path("s p a c e "));
    EXPECT_EQ("in_valid", make_safe_path("in\\\" v*[a]{l}id"));
}
