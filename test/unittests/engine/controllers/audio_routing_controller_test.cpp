#include "gtest/gtest.h"

#include "engine/audio_engine.h"
#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "engine/controller/audio_routing_controller.cpp"

using namespace sushi;
using namespace sushi::engine;
using namespace sushi::engine::controller_impl;

constexpr float TEST_SAMPLE_RATE = 44100;

class AudioRoutingControllerTest : public ::testing::Test
{
protected:
    AudioRoutingControllerTest() {}

    void SetUp()
    {
        bool debug_mode_sw = false;
        _audio_engine = std::make_unique<AudioEngine>(TEST_SAMPLE_RATE, 1, debug_mode_sw, new EventDispatcherMockup());
        _event_dispatcher_mockup = static_cast<EventDispatcherMockup*>(_audio_engine->event_dispatcher());
        _module_under_test = std::make_unique<AudioRoutingController>(_audio_engine.get());

        _audio_engine->set_audio_input_channels(8);
        _audio_engine->set_audio_output_channels(8);

        _audio_engine->create_track("Track 1", 2);
        _track_id = _audio_engine->processor_container()->track("Track 1")->id();
    }

    void TearDown() {}

    EventDispatcherMockup*                  _event_dispatcher_mockup{nullptr};
    std::unique_ptr<AudioEngine>            _audio_engine;
    std::unique_ptr<AudioRoutingController> _module_under_test;
    ObjectId _track_id;
};

TEST_F(AudioRoutingControllerTest, TestGettingAudioRouting)
{
    auto connections = _module_under_test->get_all_input_connections();
    EXPECT_EQ(0u, connections.size());
    connections = _module_under_test->get_all_output_connections();
    EXPECT_EQ(0u, connections.size());
    connections = _module_under_test->get_input_connections_for_track(static_cast<int>(_track_id));
    EXPECT_EQ(0u, connections.size());
    connections = _module_under_test->get_output_connections_for_track(static_cast<int>(_track_id));
    EXPECT_EQ(0u, connections.size());

    // Connect the track to input channels 2 & 3 and output channels 4 & 5
    auto status = _audio_engine->connect_audio_input_bus(1, 0, _track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _audio_engine->connect_audio_output_bus(2, 0, _track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    connections = _module_under_test->get_all_input_connections();
    ASSERT_EQ(2u, connections.size());
    EXPECT_EQ(2, connections[0].engine_channel);
    EXPECT_EQ(0, connections[0].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[0].track_id));
    EXPECT_EQ(3, connections[1].engine_channel);
    EXPECT_EQ(1, connections[1].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[1].track_id));

    connections = _module_under_test->get_all_output_connections();
    ASSERT_EQ(2u, connections.size());
    EXPECT_EQ(4, connections[0].engine_channel);
    EXPECT_EQ(0, connections[0].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[0].track_id));
    EXPECT_EQ(5, connections[1].engine_channel);
    EXPECT_EQ(1, connections[1].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[1].track_id));

    connections = _module_under_test->get_input_connections_for_track(static_cast<int>(_track_id));
    EXPECT_EQ(2u, connections.size());
    connections = _module_under_test->get_output_connections_for_track(static_cast<int>(_track_id));
    EXPECT_EQ(2u, connections.size());

    // Test for non-existing tracks as well
    connections = _module_under_test->get_input_connections_for_track(12345);
    EXPECT_EQ(0u, connections.size());
    connections = _module_under_test->get_output_connections_for_track(23456);
    EXPECT_EQ(0u, connections.size());
}

TEST_F(AudioRoutingControllerTest, TestSettingAudioRouting)
{
    // Connect using controller functions (using events)
    auto status = _module_under_test->connect_input_channel_to_track(_track_id, 0, 2);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status1 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    status = _module_under_test->connect_input_channel_to_track(_track_id, 1, 3);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status2 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    auto connections = _module_under_test->get_all_input_connections();
    ASSERT_EQ(2u, connections.size());
    EXPECT_EQ(2, connections[0].engine_channel);
    EXPECT_EQ(0, connections[0].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[0].track_id));
    EXPECT_EQ(3, connections[1].engine_channel);
    EXPECT_EQ(1, connections[1].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[1].track_id));

    // Do the same for output connections
    status = _module_under_test->connect_output_channel_to_track(_track_id, 0, 4);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status3 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status3, EventStatus::HANDLED_OK);

    status = _module_under_test->connect_output_channel_to_track(_track_id, 1, 5);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status4 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status4, EventStatus::HANDLED_OK);

    connections = _module_under_test->get_all_output_connections();
    ASSERT_EQ(2u, connections.size());
    EXPECT_EQ(4, connections[0].engine_channel);
    EXPECT_EQ(0, connections[0].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[0].track_id));
    EXPECT_EQ(5, connections[1].engine_channel);
    EXPECT_EQ(1, connections[1].track_channel);
    EXPECT_EQ(_track_id, ObjectId(connections[1].track_id));
}

TEST_F(AudioRoutingControllerTest, TestRemovingAudioRouting)
{
    // Connect the track to input channels 2 & 3 and output channels 4 & 5
    auto engine_status = _audio_engine->connect_audio_input_bus(1, 0, _track_id);
    ASSERT_EQ(EngineReturnStatus::OK, engine_status);
    engine_status = _audio_engine->connect_audio_output_bus(2, 0, _track_id);
    ASSERT_EQ(EngineReturnStatus::OK, engine_status);

    // Disconnect using controller functions (using events)
    auto status = _module_under_test->disconnect_input(_track_id, 0, 2);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status1 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    auto connections = _module_under_test->get_all_input_connections();
    EXPECT_EQ(1u, connections.size());
    connections = _module_under_test->get_all_output_connections();
    EXPECT_EQ(2u, connections.size());

    status = _module_under_test->disconnect_all_outputs_from_track(_track_id);
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto execution_status2 = _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    connections = _module_under_test->get_all_input_connections();
    EXPECT_EQ(1u, connections.size());
    connections = _module_under_test->get_all_output_connections();
    EXPECT_EQ(0u, connections.size());
}
