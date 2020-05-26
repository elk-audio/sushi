#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"

#define private public
#include "engine/midi_dispatcher.cpp"

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::midi_dispatcher;

class DummyMidiFrontend : public midi_frontend::BaseMidiFrontend
{
public:
    DummyMidiFrontend() : BaseMidiFrontend(nullptr) {}

    virtual ~DummyMidiFrontend() {};

    bool init() override {return true;}
    void run()  override {}
    void stop() override {}
    void send_midi(int input, MidiDataByte /*data*/, Time /*timestamp*/) override
    {
        _sent = true;
        _input = input;
    }
    bool midi_sent_on_input(int input)
    {
        if (_sent && input == _input)
        {
            _sent = false;
            return true;
        }
        return false;
    }
    private:
    bool _sent{false};
    int  _input;
};

const MidiDataByte TEST_NOTE_ON_CH2   = {0x92, 62, 55, 0}; /* Channel 2 */
const MidiDataByte TEST_NOTE_OFF_CH3  = {0x83, 60, 45, 0}; /* Channel 3 */
const MidiDataByte TEST_CTRL_CH_CH4   = {0xB4, 67, 75, 0}; /* Channel 4, cc 67 */
const MidiDataByte TEST_CTRL_CH_CH5_2 = {0xB5, 40, 75, 0}; /* Channel 5, cc 40 */
const MidiDataByte TEST_CTRL_CH_CH5_3 = {0xB5, 39, 75, 0}; /* Channel 5, cc 39 */
const MidiDataByte TEST_PRG_CH_CH5   =  {0xC5, 40, 0, 0};  /* Channel 5, prg 40 */
const MidiDataByte TEST_PRG_CH_CH4_2  = {0xC4, 45, 0, 0};  /* Channel 4, prg 45 */

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
    EXPECT_TRUE(event->is_parameter_change_event());
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
        _test_dispatcher = static_cast<EventDispatcherMockup*>(_test_engine.event_dispatcher());
        _module_under_test.set_frontend(&_test_frontend);
    }

    void TearDown()
    {
    }
    EngineMockup _test_engine{48000};
    MidiDispatcher _module_under_test{&_test_engine};
    EventDispatcherMockup* _test_dispatcher;
    DummyMidiFrontend _test_frontend;
};

TEST_F(TestMidiDispatcher, TestKeyboardDataConnection)
{
    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_kb_to_track(1, "processor");
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_kb_from_track(1, "processor");

    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // TODO/Q: Should I keep this at all?
    // _module_under_test.clear_connections();

    /* Connect with a specific midi channel (2) */
    _module_under_test.connect_kb_to_track(2, "processor_2", 3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect specific midi channel */
    _module_under_test.disconnect_kb_from_track(2, "processor_2", 3);

    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestMidiDispatcher, TestKeyboardDataOutConnection)
{
    KeyboardEvent event_ch12(KeyboardEvent::Subtype::NOTE_ON,
                             5,
                             12,
                             48,
                             0.5f,
                             IMMEDIATE_PROCESS);

    KeyboardEvent event_ch5(KeyboardEvent::Subtype::NOTE_ON,
                            1,
                            5,
                            48,
                            0.5f,
                            IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status = _module_under_test.process(&event_ch12);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));

    /* Connect track to output 1, channel 5 */
    _module_under_test.set_midi_outputs(3);
    auto ret = _module_under_test.connect_track_to_output(1, "processor", 5);
    ASSERT_EQ(MidiDispatcherStatus::OK, ret);

    status = _module_under_test.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));

    // TODO/Q: This test fails when running tests in batch, but succeeds when run individually.
    // I introduced it, as I thought it was missing by omission.
    // But maybe there was a reason why sending midi out cannot be tested?
    //EXPECT_TRUE(_test_frontend.midi_sent_on_input(1));

    ret = _module_under_test.disconnect_track_from_output(1, "processor", 5);
    ASSERT_EQ(MidiDispatcherStatus::OK, ret);

    status = _module_under_test.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(1));

    status = _module_under_test.process(&event_ch12);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(1));
}

TEST_F(TestMidiDispatcher, TestRawDataConnection)
{
    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_raw_midi_to_track(1, "processor");
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_raw_midi_from_track(1, "processor");
    _module_under_test.send_midi(1, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // TODO/Q: Should I keep this at all?
    //_module_under_test.clear_connections();

    /* Connect with a specific midi channel (2) */
    _module_under_test.connect_raw_midi_to_track(2, "processor_2", 3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_CH2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect specific midi channel */
    _module_under_test.disconnect_raw_midi_from_track(2, "processor", 3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestMidiDispatcher, TestCCDataConnection)
{
    /* Test with no connections set */
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(5, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_cc_to_parameter(1,
                                               "processor",
                                               "parameter",
                                               67,
                                               0,
                                               false,
                                               100);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    /* Send on a different input and a msg with a different cc no */
    _module_under_test.send_midi(5, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_cc_from_parameter(1,
                                                    "processor",
                                                    67);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // TODO: Remove or reinstate
    //_module_under_test.clear_connections();

    /* Connect with a specific midi channel (5) */
    _module_under_test.connect_cc_to_parameter(1,
                                               "processor",
                                               "parameter",
                                               40,
                                               0,
                                               100,
                                               false,
                                               5);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH4, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(2, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect specific channel */
    _module_under_test.disconnect_cc_from_parameter(1,
                                                    "processor",
                                                    40,
                                                    5);

    _module_under_test.send_midi(1, TEST_CTRL_CH_CH5_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestMidiDispatcher, TestProgramChangeConnection)
{
    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_inputs(5);
    _module_under_test.connect_pc_to_processor(1, "processor");
    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect OMNI */
    _module_under_test.disconnect_pc_from_processor(1, "processor");

    _module_under_test.send_midi(1, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // TODO: Reinstate or remove
    //_module_under_test.clear_connections();

    /* Connect with a specific midi channel (4) */
    _module_under_test.connect_pc_to_processor(2, "processor_2", 4);
    _module_under_test.send_midi(2, TEST_PRG_CH_CH4_2, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(2, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Disconnect specific channel */
    _module_under_test.disconnect_pc_from_processor(2, "processor_2", 4);
    _module_under_test.send_midi(2, TEST_PRG_CH_CH4_2, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}
