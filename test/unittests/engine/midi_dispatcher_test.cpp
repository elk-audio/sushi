#include "gtest/gtest.h"
#define private public

#include "engine/midi_dispatcher.cpp"
#include "../test/unittests/audio_frontends/engine_mockup.h"

using namespace midi;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::midi_dispatcher;

const uint8_t TEST_NOTE_ON_MSG[]   = {0x92, 62, 55}; /* Channel 2 */
const uint8_t TEST_NOTE_OFF_MSG[]  = {0x83, 60, 45}; /* Channel 3 */
const uint8_t TEST_CTRL_CH_MSG[]   = {0xB4, 67, 75}; /* Channel 4, cc 67 */
const uint8_t TEST_CTRL_CH_MSG_2[] = {0xB5, 40, 75}; /* Channel 5, cc 40 */
const uint8_t TEST_CTRL_CH_MSG_3[] = {0xB5, 39, 75}; /* Channel 5, cc 39 */


// mockup functions in the engine:

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOnEvent)
{
    Connection connection = {25, 26, 0, 1};
    NoteOnMessage message = {1, 46, 64};
    RtEvent event = make_note_on_event(connection, message, 10);
    EXPECT_EQ(RtEventType::NOTE_ON, event.type());
    auto typed_event = event.keyboard_event();
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(10, typed_event->sample_offset());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
}

TEST(TestMidiDispatcherEventCreation, TestMakeNoteOffEvent)
{
    Connection connection = {25, 26, 0, 1};
    NoteOffMessage message = {1, 46, 64};
    RtEvent event = make_note_off_event(connection, message, 10);
    EXPECT_EQ(RtEventType::NOTE_OFF, event.type());
    auto typed_event = event.keyboard_event();
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(10, typed_event->sample_offset());
    EXPECT_EQ(46, typed_event->note());
    EXPECT_NEAR(0.5, typed_event->velocity(), 0.05);
}

TEST(TestMidiDispatcherEventCreation, TestMakeParameterChangeEvent)
{
    Connection connection = {25, 26, 0, 1};
    ControlChangeMessage message = {1, 50, 32};
    RtEvent event = make_param_change_event(connection, message, 10);
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    auto typed_event = event.parameter_change_event();
    EXPECT_EQ(25u, typed_event->processor_id());
    EXPECT_EQ(26u, typed_event->param_id());
    EXPECT_EQ(10, typed_event->sample_offset());
    EXPECT_NEAR(0.25, typed_event->value(), 0.01);
}

class TestMidiDispatcher : public ::testing::Test
{
protected:
    TestMidiDispatcher()
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
    EngineMockup _test_engine{41000};
    MidiDispatcher _module_under_test{&_test_engine};
};

TEST_F(TestMidiDispatcher, TestKeyboardDataConnection)
{
    _test_engine.got_event = false;
    /* Send midi message without connections */
    _module_under_test.process_midi(1, 0, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), false);
    _module_under_test.process_midi(0, 0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), false);
    EXPECT_FALSE(_test_engine.got_event);

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_input_ports(5);
    _module_under_test.connect_kb_to_track(1, "processor");
    _module_under_test.process_midi(1, 0, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), false);
    EXPECT_TRUE(_test_engine.got_event);
    _test_engine.got_event = false;
    _module_under_test.process_midi(0, 0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), false);
    EXPECT_FALSE(_test_engine.got_event);
    _module_under_test.clear_connections();

    /* Connect with a specific midi channel (2) */
    _module_under_test.connect_kb_to_track(2, "processor_2", 3);
    _module_under_test.process_midi(2, 0, TEST_NOTE_OFF_MSG, sizeof(TEST_NOTE_OFF_MSG), false);
    EXPECT_TRUE(_test_engine.got_event);
    _test_engine.got_event = false;
    _module_under_test.process_midi(2, 0, TEST_NOTE_ON_MSG, sizeof(TEST_NOTE_ON_MSG), false);
    EXPECT_FALSE(_test_engine.got_event);
}

TEST_F(TestMidiDispatcher, TestCCDataConnection)
{
    _test_engine.got_event = false;
    /* Test with no connections set */
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), false);
    _module_under_test.process_midi(5, 0, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), false);
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    EXPECT_FALSE(_test_engine.got_event);

    /* Connect all midi channels (OMNI) */
    _module_under_test.set_midi_input_ports(5);
    _module_under_test.connect_cc_to_parameter(1, "processor", "parameter", 67, 0, 100);
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), false);
    EXPECT_TRUE(_test_engine.got_event);
    _test_engine.got_event = false;
    /* Send on a different input and a msg with a different cc no */
    _module_under_test.process_midi(5, 0, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), false);
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    EXPECT_FALSE(_test_engine.got_event);
    _module_under_test.clear_connections();

    /* Connect with a specific midi channel (5) */
    _module_under_test.connect_cc_to_parameter(1, "processor", "parameter", 40, 0, 100, 5);
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    EXPECT_TRUE(_test_engine.got_event);
    _test_engine.got_event = false;
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG, sizeof(TEST_CTRL_CH_MSG), false);
    _module_under_test.process_midi(2, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    _module_under_test.process_midi(1, 0, TEST_CTRL_CH_MSG_3, sizeof(TEST_CTRL_CH_MSG_3), false);
    EXPECT_FALSE(_test_engine.got_event);
}

TEST_F(TestMidiDispatcher, TestRtAndNonRtDispatch)
{
    /* Connect midi channel (5) and send with realtime = false */
    _module_under_test.set_midi_input_ports(1);
    _module_under_test.connect_cc_to_parameter(0, "processor", "parameter", 40, 0, 100, 5);
    _module_under_test.process_midi(0, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), false);
    EXPECT_TRUE(_test_engine.got_event);
    EXPECT_FALSE(_test_engine.got_rt_event);

    /* Sent with realtime = true */
    _test_engine.got_event = false;
    _module_under_test.process_midi(0, 0, TEST_CTRL_CH_MSG_2, sizeof(TEST_CTRL_CH_MSG_2), true);
    EXPECT_FALSE(_test_engine.got_event);
    EXPECT_TRUE(_test_engine.got_rt_event);
}
