#include "gtest/gtest.h"
#define private public

#include "engine/midi_dispatcher.cpp"
#include "test_utils/engine_mockup.h"

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
    void send_midi(int /*input*/, const uint8_t* /*data*/, Time /*timestamp*/) override
    {
        _sent = true;
    }
    bool midi_sent()
    {
        if (_sent)
        {
            _sent = false;
            return true;
        }
        return false;
    }
    private:
    bool _sent{false};
};

const uint8_t TEST_NOTE_ON_MSG[]   = {0x92, 62, 55}; /* Channel 2 */
const uint8_t TEST_NOTE_OFF_MSG[]  = {0x83, 60, 45}; /* Channel 3 */
const uint8_t TEST_CTRL_CH_MSG[]   = {0xB4, 67, 75}; /* Channel 4, cc 67 */
const uint8_t TEST_CTRL_CH_MSG_2[] = {0xB5, 40, 75}; /* Channel 5, cc 40 */
const uint8_t TEST_CTRL_CH_MSG_3[] = {0xB5, 39, 75}; /* Channel 5, cc 39 */


TEST(TestMidiDispatcherEventCreation, TestMakeNoteOnEvent)
{
    InputConnection connection = {25, 26, 0, 1};
    NoteOnMessage message = {1, 46, 64};
    Event* event = make_note_on_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_ON, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOffEvent)
{
    InputConnection connection = {25, 26, 0, 1};
    NoteOffMessage message = {1, 46, 64};
    Event* event = make_note_off_event(connection, message, IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::NOTE_OFF, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeWrappedMidiEvent)
{
    InputConnection connection = {25, 26, 0, 1};
    uint8_t message[] = {1, 46, 64};
    Event* event = make_wrapped_midi_event(connection, message, sizeof(message), IMMEDIATE_PROCESS);
    EXPECT_TRUE(event->is_keyboard_event());
    EXPECT_EQ(IMMEDIATE_PROCESS, event->time());
    auto typed_event = static_cast<KeyboardEvent*>(event);
    EXPECT_EQ(KeyboardEvent::Subtype::WRAPPED_MIDI, typed_event->subtype());
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(1u, typed_event->midi_data()[0]);
    EXPECT_EQ(46u, typed_event->midi_data()[1]);
    EXPECT_EQ(64u, typed_event->midi_data()[2]);
    EXPECT_EQ(0u, typed_event->midi_data()[3]);
    delete event;
}

TEST(TestMidiDispatcherEventCreation, TestMakeParameterChangeEvent)
{
    InputConnection connection = {25, 26, 0, 1};
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
    EngineMockup _test_engine{41000};
    MidiDispatcher _module_under_test{&_test_engine};
    EventDispatcherMockup* _test_dispatcher;
    DummyMidiFrontend _test_frontend;
};

TEST_F(TestMidiDispatcher, TestKeyboardDataConnection)
{
    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_input_ports(5);
    _module_under_test.connect_kb_to_track(1, "processor");
    _module_under_test.send_midi(1, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
    _module_under_test.clear_connections();

    /* Connect with a specific midi channel (2) */
    _module_under_test.connect_kb_to_track(2, "processor_2", 3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestMidiDispatcher, TestKeyboardDataOutConnection)
{
    KeyboardEvent event(KeyboardEvent::Subtype::NOTE_ON, 0, 12, 0.5f, IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status = _module_under_test.process(&event);
    EXPECT_EQ(EventStatus::HANDLED_OK, status);
    EXPECT_FALSE(_test_frontend.midi_sent());

    /* Connect track to output 1, channel 5 */
    _module_under_test.set_midi_output_ports(3);
    auto ret = _module_under_test.connect_track_to_output(1, "processor", 5);
    ASSERT_EQ(MidiDispatcherStatus::OK, ret);
}


TEST_F(TestMidiDispatcher, TestRawDataConnection)
{
    /* Send midi message without connections */
    _module_under_test.send_midi(1, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_input_ports(5);
    _module_under_test.connect_raw_midi_to_track(1, "processor");
    _module_under_test.send_midi(1, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
    _module_under_test.clear_connections();

    /* Connect with a specific midi channel (2) */
    _module_under_test.connect_raw_midi_to_track(2, "processor_2", 3);
    _module_under_test.send_midi(2, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(2, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(TestMidiDispatcher, TestCCDataConnection)
{
    /* Test with no connections set */
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(5, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_input_ports(5);
    _module_under_test.connect_cc_to_parameter(1, "processor", "parameter", 67, 0, 100);
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    /* Send on a different input and a msg with a different cc no */
    _module_under_test.send_midi(5, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _module_under_test.clear_connections();

    /* Connect with a specific midi channel (5) */
    _module_under_test.connect_cc_to_parameter(1, "processor", "parameter", 40, 0, 100, 5);
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(2, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), IMMEDIATE_PROCESS);
    _module_under_test.send_midi(1, TEST_CTRL_CH_MSG_3, sizeof(TEST_CTRL_CH_MSG_3), IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

/*TEST_F(TestMidiDispatcher, TestRtAndNonRtDispatch)
{
    _module_under_test.set_midi_input_ports(1);
    _module_under_test.connect_cc_to_parameter(0, "processor", "parameter", 40, 0, 100, 5);
    _module_under_test.send_midi(0, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    EXPECT_TRUE(_test_engine.got_event);
    EXPECT_FALSE(_test_engine.got_rt_event);

    _test_engine.got_event = false;
    _module_under_test.send_midi(0, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), true);
    EXPECT_FALSE(_test_engine.got_event);
    EXPECT_TRUE(_test_engine.got_rt_event);
}*/
