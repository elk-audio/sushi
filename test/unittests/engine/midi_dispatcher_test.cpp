#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/mock_midi_frontend.h"

#define private public
#include "engine/midi_dispatcher.cpp"

using ::testing::NiceMock;
using ::testing::_;

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::midi_dispatcher;

const MidiDataByte TEST_NOTE_ON_CH2   = {0x91, 62, 55, 0}; /* Channel 2 */
const MidiDataByte TEST_NOTE_OFF_CH3  = {0x82, 60, 45, 0}; /* Channel 3 */
const MidiDataByte TEST_CTRL_CH_CH4_67 = {0xB3, 67, 75, 0}; /* Channel 4, cc 67 */
const MidiDataByte TEST_CTRL_CH_CH4_68 = {0xB3, 68, 75, 0}; /* Channel 4, cc 68 */
const MidiDataByte TEST_CTRL_CH_CH5_2 = {0xB4, 40, 75, 0}; /* Channel 5, cc 40 */
const MidiDataByte TEST_CTRL_CH_CH5_3 = {0xB4, 39, 75, 0}; /* Channel 5, cc 39 */
const MidiDataByte TEST_PRG_CH_CH5   =  {0xC4, 40, 0, 0};  /* Channel 5, prg 40 */
const MidiDataByte TEST_PRG_CH_CH4_2  = {0xC3, 45, 0, 0};  /* Channel 4, prg 45 */

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOnEvent)
{
    InputConnection connection = {25, 26, 0, 1, false, 64};
    NoteOnMessage message = {1, 46, 64};
    Event* event = make_note_on_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(1, typed_event->channel());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOnWithZeroVelEvent)
{
    InputConnection connection = {25, 26, 0, 1, false, 64};
    NoteOnMessage message = {1, 60, 0};
    Event* event = make_note_on_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(1, typed_event->channel());
    EXPECT_EQ(60, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOffEvent)
{
    InputConnection connection = {25, 26, 0, 1, false, 64};
    NoteOffMessage message = {2, 46, 64};
    Event* event = make_note_off_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(2, typed_event->channel());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeWrappedMidiEvent)
{
    InputConnection connection = {25, 26, 0, 1, false, 64};
    uint8_t message[] = {3, 46, 64};
    Event* event = make_wrapped_midi_event(connection, message, sizeof(message), IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::WRAPPED_MIDI, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(3u, typed_event->midi_data()[0]);
    EXPECT_EQ(46u, typed_event->midi_data()[1]);
    EXPECT_EQ(64u, typed_event->midi_data()[2]);
    EXPECT_EQ(0u, typed_event->midi_data()[3]);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeParameterChangeEvent)
{
    InputConnection connection = {25, 26, 0, 1, false, 64};
    ControlChangeMessage message = {1, 50, 32};
    Event* event = make_param_change_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<ParameterChangeEvent*>(event);
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(26u, typed_event->parameter_id());
    EXPECT_NEAR(0.25, typed_event->float_value(), 0.01);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeProgramChangeEvent)
{
    InputConnection connection = {25, 0, 0, 0, false, 64};
    ProgramChangeMessage message = {1, 32};
    Event* event = make_program_change_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<ProgramChangeEvent*>(event);
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(32, typed_event->program_no());
    delete event;
}

class TestMidiDispatcher : public ::testing::Test
{
protected:
    TestMidiDispatcher()
    {
    }

    void SetUp()
    {
        _module_under_test.set_frontend(&_mock_frontend);
    }

    void TearDown()
    {
    }

    EngineMockup _test_engine{48000};
    EventDispatcherMockup _test_dispatcher;
    ::testing::NiceMock<MockMidiFrontend> _mock_frontend{nullptr};
    MidiDispatcher _module_under_test{&_test_dispatcher};
};

TEST_F(TestMidiDispatcher, TestKeyboardDataConnection)
{
    auto track_1 = _test_engine.processor_container()->track("track 1");
    ObjectId track_id_1 = track_1->id();
    auto track_2 = _test_engine.processor_container()->track("track 2");
    ObjectId track_id_2 = track_2->id();

    auto input_connections = _module_under_test.get_all_kb_input_connections();
    EXPECT_TRUE(input_connections.size()==0);

    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_kb_to_track(1, track_id_1);
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_kb_from_track(1, track_id_1);

    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect with a specific midi channel (3) */
    _module_under_test.connect_kb_to_track(2,
                                           track_id_2,
                                           midi::MidiChannel::CH_3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Test fetching connections */
    input_connections = _module_under_test.get_all_kb_input_connections();
    EXPECT_TRUE(input_connections.size() == 1);

    /* Disconnect specific midi channel */
    _module_under_test.disconnect_kb_from_track(2,
                                                track_id_2,
                                                midi::MidiChannel::CH_3);

    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    input_connections = _module_under_test.get_all_kb_input_connections();
    EXPECT_TRUE(input_connections.size() == 0);
}

TEST_F(TestMidiDispatcher, TestKeyboardDataOutConnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    auto output_connections = _module_under_test.get_all_kb_output_connections();
    EXPECT_TRUE(output_connections.size()==0);

    KeyboardEvent event_ch12(KeyboardEvent::Subtype::NOTE_ON,
                             track_id,
                             12,
                             48,
                             0.5f,
                             IMMEDIATE_PROCESS);

    KeyboardEvent event_ch5(KeyboardEvent::Subtype::NOTE_ON,
                            track_id,
                            5,
                            48,
                            0.5f,
                            IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status = _module_under_test.process(&event_ch12);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);

    /* Connect track to output 1, channel 5 */
    _module_under_test.set_midi_outputs(3);
    auto ret = _module_under_test.connect_track_to_output(1,
                                                          track_id,
                                                          midi::MidiChannel::CH_5);
    ASSERT_EQ(MidiDispatcherStatus::OK, ret);

    /* Expect a midi output message */
    EXPECT_CALL(_mock_frontend, send_midi(1, midi::encode_note_on(4, 48, 0.5f), _)).Times(1);
    status = _module_under_test.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);

    output_connections = _module_under_test.get_all_kb_output_connections();
    EXPECT_TRUE(output_connections.size() == 1);

    ret = _module_under_test.disconnect_track_from_output(1,
                                                          track_id,
                                                          midi::MidiChannel::CH_5);
    ASSERT_EQ(MidiDispatcherStatus::OK, ret);

    status = _module_under_test.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);

    status = _module_under_test.process(&event_ch12);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);

    output_connections = _module_under_test.get_all_kb_output_connections();
    EXPECT_TRUE(output_connections.size() == 0);
}

TEST_F(TestMidiDispatcher, TestTransportOutputs)
{
    _module_under_test.set_midi_outputs(2);
    EXPECT_FALSE(_module_under_test.midi_clock_enabled(0));
    EXPECT_FALSE(_module_under_test.midi_clock_enabled(1));
    auto status = _module_under_test.enable_midi_clock(true, 1);
    EXPECT_EQ(MidiDispatcherStatus::OK, status);
    status = _module_under_test.enable_midi_clock(true, 123);
    EXPECT_NE(MidiDispatcherStatus::OK, status);
    EXPECT_FALSE(_module_under_test.midi_clock_enabled(0));
    EXPECT_TRUE(_module_under_test.midi_clock_enabled(1));

    auto start_event = PlayingModeNotificationEvent(PlayingMode::PLAYING, IMMEDIATE_PROCESS);
    auto stop_event = PlayingModeNotificationEvent(PlayingMode::STOPPED, IMMEDIATE_PROCESS);
    auto rec_event = PlayingModeNotificationEvent(PlayingMode::RECORDING, IMMEDIATE_PROCESS);
    auto tick_event = EngineTimingTickNotificationEvent(0, IMMEDIATE_PROCESS);

    EXPECT_CALL(_mock_frontend, send_midi(1, midi::encode_start_message(), _)).Times(1);
    EXPECT_CALL(_mock_frontend, send_midi(1, midi::encode_stop_message(), _)).Times(1);
    EXPECT_CALL(_mock_frontend, send_midi(1, midi::encode_timing_clock(), _)).Times(1);

    _module_under_test.process(&start_event);
    _module_under_test.process(&stop_event);
    _module_under_test.process(&rec_event);
    _module_under_test.process(&tick_event);
}

TEST_F(TestMidiDispatcher, TestRawDataConnection)
{
    auto track_1 = _test_engine.processor_container()->track("track 1");
    ObjectId track_id_1 = track_1->id();
    auto track_2 = _test_engine.processor_container()->track("track 2");
    ObjectId track_id_2 = track_2->id();

    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_raw_midi_to_track(1, track_id_1);
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_raw_midi_from_track(1, track_id_1);
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect with a specific midi channel (3) */
    _module_under_test.connect_raw_midi_to_track(2,
                                                 track_id_2,
                                                 midi::MidiChannel::CH_3);

    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Disconnect specific midi channel */
    _module_under_test.disconnect_raw_midi_from_track(2,
                                                      track_id_1,
                                                      midi::MidiChannel::CH_3);

    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());
}

TEST_F(TestMidiDispatcher, TestCCDataConnection)
{
    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    const auto parameter = processor->parameter_from_name("param 1");
    ObjectId parameter_id = parameter->id();

    /* Test with no connections set */
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(5, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_cc_to_parameter(1,
                                               processor_id,
                                               parameter_id,
                                               67,
                                               0,
                                               100,
                                               false);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    /* Send on a different input and a msg with a different cc no */
    _module_under_test.send_midi(5, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_cc_from_parameter(1,
                                                    processor_id,
                                                    67);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect with a specific midi channel (5) */
    _module_under_test.connect_cc_to_parameter(1,
                                               processor_id,
                                               parameter_id,
                                               40,
                                               0,
                                               100,
                                               false,
                                               midi::MidiChannel::CH_5);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(2, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    _module_under_test.connect_cc_to_parameter(1,
                                               processor_id,
                                               parameter_id,
                                               68,
                                               0,
                                               100,
                                               false,
                                               midi::MidiChannel::CH_4);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    /* Test fetching connections */
    auto input_connections = _module_under_test.get_all_cc_input_connections();
    EXPECT_TRUE(input_connections.size()==2);

    /* Tests fetching using a non-existent processor ID */
    auto input_connection = _module_under_test.get_cc_input_connections_for_processor(1);
    EXPECT_TRUE(input_connection.size()==0);

    /* Disconnect specific channel */
    _module_under_test.disconnect_cc_from_parameter(1,
                                                    processor_id,
                                                    40,
                                                    midi::MidiChannel::CH_5);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());
}

TEST_F(TestMidiDispatcher, TestProgramChangeConnection)
{
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_pc_to_processor(1, processor_id);
    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_pc_from_processor(1, processor_id);

    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Connect with a specific midi channel (4) */
    _module_under_test.connect_pc_to_processor(2,
                                               processor_id,
                                               midi::MidiChannel::CH_4);

    _module_under_test.send_midi(2, TEST_PRG_CH_CH4_2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher.got_event());

    _module_under_test.send_midi(2, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());

    /* Test fetching connections */
    auto input_connections = _module_under_test.get_all_pc_input_connections();
    EXPECT_TRUE(input_connections.size()==1);

    /* Tests fetching using a non-existent processor ID */
    auto input_connection = _module_under_test.get_pc_input_connections_for_processor(2000);
    EXPECT_TRUE(input_connection.size()==0);

    /* Disconnect specific channel */
    _module_under_test.disconnect_pc_from_processor(2,
                                                     processor_id,
                                                     midi::MidiChannel::CH_4);

    _module_under_test.send_midi(2, TEST_PRG_CH_CH4_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher.got_event());
}