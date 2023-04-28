#include "gtest/gtest.h"

#include "engine/audio_engine.h"
#include "library/midi_encoder.h"

#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"
#include "test_utils/mock_midi_frontend.h"
#include "engine/controller/midi_controller.cpp"

using ::testing::NiceMock;
using ::testing::_;

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::control_frontend;
using namespace sushi::midi_dispatcher;
using namespace sushi::engine::controller_impl;

constexpr float TEST_SAMPLE_RATE = 44100;

const MidiDataByte TEST_NOTE_OFF_CH3 = {0x82, 60, 45, 0}; /* Channel 3 */
const MidiDataByte TEST_CTRL_CH_CH4_67 = {0xB3, 67, 75, 0}; /* Channel 4, cc 67 */
const MidiDataByte TEST_CTRL_CH_CH4_68 = {0xB3, 68, 75, 0}; /* Channel 4, cc 68 */
const MidiDataByte TEST_CTRL_CH_CH4_70 = {0xB3, 70, 75, 0}; /* Channel 4, cc 70 */
const MidiDataByte TEST_PRG_CH_CH5 = {0xC4, 40, 0, 0};  /* Channel 5, prg 40 */
const MidiDataByte TEST_PRG_CH_CH6 = {0xC5, 40, 0, 0};  /* Channel 6, prg 40 */
const MidiDataByte TEST_PRG_CH_CH7 = {0xC6, 40, 0, 0};  /* Channel 7, prg 40 */

class MidiControllerEventTestFrontend : public ::testing::Test
{
protected:
    MidiControllerEventTestFrontend() {}

    void SetUp()
    {
        _test_dispatcher = static_cast<EventDispatcherMockup*>(_test_engine.event_dispatcher());
        _midi_dispatcher.set_frontend(&_mock_frontend);
    }

    void TearDown() {}

    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    MidiDispatcher _midi_dispatcher{_test_engine.event_dispatcher()};
    sushi::ext::ControlMockup _controller; // TODO: Maybe just the ParameterControllerMockup?
    MidiController _midi_controller{&_test_engine, &_midi_dispatcher};
    EventDispatcherMockup* _test_dispatcher;
    ::testing::NiceMock<MockMidiFrontend> _mock_frontend{nullptr};
};

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();
    bool raw_midi = false;
    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto event_status_connect = _midi_controller.connect_kbd_input_to_track(track_id, channel, port, raw_midi);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect);
    // That the engine is passed as argument to execute violates the Liskov Substitution Principle and should not be necessary.
    // A refactor to how events work would solve that.

    auto execution_status1 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto event_status_disconnect =  _midi_controller.disconnect_kbd_input(track_id, channel, port, raw_midi);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect);
    auto execution_status2 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnectionRaw)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();
    bool raw_midi = true;
    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    auto event_status_connect = _midi_controller.connect_kbd_input_to_track(track_id, channel, port, raw_midi);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect);
    auto execution_status1 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto event_status_disconnect =  _midi_controller.disconnect_kbd_input(track_id, channel, port, raw_midi);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect);
    auto execution_status2 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdOutputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();
    int port = 0;

    _midi_dispatcher.set_midi_outputs(5);

    ext::MidiChannel channel_3 = sushi::ext::MidiChannel::MIDI_CH_3;

    int int_channel_3 = int_from_ext_midi_channel(channel_3);

    KeyboardEvent event_ch3(KeyboardEvent::Subtype::NOTE_ON,
                            track_id,
                            int_channel_3,
                            48,
                            0.5f,
                            IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status1 = _midi_dispatcher.process(&event_ch3);
    EXPECT_EQ(EventStatus::HANDLED_OK, status1);

    auto event_status_connect = _midi_controller.connect_kbd_output_from_track(track_id, channel_3, port);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect);
    auto execution_status1 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    EXPECT_CALL(_mock_frontend, send_midi(0, midi::encode_note_on(2, 48, 0.5f), _)).Times(1);
    auto status2 = _midi_dispatcher.process(&event_ch3);
    EXPECT_EQ(EventStatus::HANDLED_OK, status2);

    auto event_status_disconnect =  _midi_controller.disconnect_kbd_output(track_id, channel_3, port);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect);
    auto execution_status2 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    auto status3 = _midi_dispatcher.process(&event_ch3);
    EXPECT_EQ(EventStatus::HANDLED_OK, status3);
}

TEST_F(MidiControllerEventTestFrontend, TestCCDataConnectionDisconnection)
{
    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_4;
    int port = 0;

    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    const auto parameter = processor->parameter_from_name("param 1");
    ObjectId parameter_id = parameter->id();

    _midi_dispatcher.set_midi_inputs(5);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 67:

    auto event_status_connect1 = _midi_controller.connect_cc_to_parameter(processor_id,
                                                                          parameter_id,
                                                                          channel,
                                                                          port,
                                                                          67, // cc_number
                                                                          0, // min_range
                                                                          100, // max_range
                                                                          false); // use_relative_mode
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect1);
    auto execution_status1 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    // Connect CC Number 68:

    auto event_status_connect2 = _midi_controller.connect_cc_to_parameter(processor_id,
                                                                          parameter_id,
                                                                          channel,
                                                                          port,
                                                                          68, // cc_number
                                                                          0, // min_range
                                                                          100, // max_range
                                                                          false); // use_relative_mode
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect2);
    auto execution_status2 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 70:

    auto event_status_connect3 = _midi_controller.connect_cc_to_parameter(processor_id,
                                                                          parameter_id,
                                                                          channel,
                                                                          port,
                                                                          70, // cc_number
                                                                          0, // min_range
                                                                          100, // max_range
                                                                          false); // use_relative_mode
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect3);
    auto execution_status3 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status3, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect CC Number 67 only:

    auto event_status_disconnect = _midi_controller.disconnect_cc(processor_id,
                                                                  channel,
                                                                  port,
                                                                  67); // cc_number

    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect);
    auto execution_status_disconnect = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status_disconnect, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all remaining CC's:

    auto event_status_disconnect_all = _midi_controller.disconnect_all_cc_from_processor(processor_id);

    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect_all);
    auto execution_status_disconnect_all = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status_disconnect_all, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestPCDataConnectionDisconnection)
{
    int port = 0;

    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    _midi_dispatcher.set_midi_inputs(5);

    // Connect Channel 5:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto event_status_connect1 = _midi_controller.connect_pc_to_processor(processor_id,
                                                                          sushi::ext::MidiChannel::MIDI_CH_5,
                                                                          port);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect1);
    auto execution_status1 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status1, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 6:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto event_status_connect2 = _midi_controller.connect_pc_to_processor(processor_id,
                                                                          sushi::ext::MidiChannel::MIDI_CH_6,
                                                                          port);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect2);
    auto execution_status2 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status2, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 7:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto event_status_connect3 = _midi_controller.connect_pc_to_processor(processor_id,
                                                                          sushi::ext::MidiChannel::MIDI_CH_7,
                                                                          port);
    ASSERT_EQ(ext::ControlStatus::OK, event_status_connect3);
    auto execution_status3 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status3, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect Channel 5 only:

    auto event_status_disconnect1 = _midi_controller.disconnect_pc(processor_id,
                                                                   sushi::ext::MidiChannel::MIDI_CH_5,
                                                                   port);

    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect1);
    auto execution_status4 = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status4, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all channels:

    auto event_status_disconnect_all = _midi_controller.disconnect_all_pc_from_processor(processor_id);

    ASSERT_EQ(ext::ControlStatus::OK, event_status_disconnect_all);
    auto execution_status_disconnect_all = _test_dispatcher->execute_engine_event(&_test_engine);
    ASSERT_EQ(execution_status_disconnect_all, EventStatus::HANDLED_OK);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestSettingClockOutput)
{
    int port = 0;
    _midi_dispatcher.set_midi_outputs(1);
    EXPECT_EQ(ext::ControlStatus::OK, _midi_controller.set_midi_clock_output_enabled(true, port));
    EXPECT_EQ(EventStatus::HANDLED_OK, _test_dispatcher->execute_engine_event(&_test_engine));

    EXPECT_EQ(ext::ControlStatus::OK, _midi_controller.set_midi_clock_output_enabled(true, 1234));
    EXPECT_NE(EventStatus::HANDLED_OK, _test_dispatcher->execute_engine_event(&_test_engine));

    _midi_dispatcher.enable_midi_clock(true, port);
    EXPECT_TRUE(_midi_controller.get_midi_clock_output_enabled(port));
    EXPECT_FALSE(_midi_controller.get_midi_clock_output_enabled(1234));
}