#include "gtest/gtest.h"

#include "engine/controller/midi_controller_events.cpp"

#undef private

#include "engine/audio_engine.h"

#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/control_mockup.h"

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
        _midi_dispatcher.set_frontend(&_test_frontend);
    }

    void TearDown()
    {

    }

    EngineMockup _test_engine{TEST_SAMPLE_RATE};
    MidiDispatcher _midi_dispatcher{_test_engine.event_dispatcher(), _test_engine.processor_container()};
    EventDispatcherMockup* _test_dispatcher;
    DummyMidiFrontend _test_frontend;
    sushi::ext::ControlMockup _controller;
};

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    const bool raw_midi = false; // Do another with true?

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    auto connection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                           track_id,
                                                           channel,
                                                           port,
                                                           raw_midi,
                                                           KbdInputToTrackConnectionEvent::Action::Connect,
                                                           IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto disconnection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                              track_id,
                                                              channel,
                                                              port,
                                                              raw_midi,
                                                              KbdInputToTrackConnectionEvent::Action::Disconnect,
                                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdInputConectionDisconnectionRaw)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    const bool raw_midi = true; // Do another with true?

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 1;

    _midi_dispatcher.set_midi_inputs(5);

    auto connection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                           track_id,
                                                           channel,
                                                           port,
                                                           raw_midi,
                                                           KbdInputToTrackConnectionEvent::Action::Connect,
                                                           IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    auto disconnection_event = KbdInputToTrackConnectionEvent(&_midi_dispatcher,
                                                              track_id,
                                                              channel,
                                                              port,
                                                              raw_midi,
                                                              KbdInputToTrackConnectionEvent::Action::Disconnect,
                                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_NOTE_OFF_CH3, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}

TEST_F(MidiControllerEventTestFrontend, TestKbdOutputConectionDisconnection)
{
    auto track = _test_engine.processor_container()->track("track 1");
    ObjectId track_id = track->id();

    KeyboardEvent event_ch5(KeyboardEvent::Subtype::NOTE_ON,
                            1,
                            5,
                            48,
                            0.5f,
                            IMMEDIATE_PROCESS);

    /* Send midi message without connections */
    auto status1 = _midi_dispatcher.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status1);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));

    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_3;
    int port = 0;

    _midi_dispatcher.set_midi_outputs(5);

    auto connection_event = KbdOutputToTrackConnectionEvent(&_midi_dispatcher,
                                                            track_id,
                                                            channel,
                                                            port,
                                                            KbdOutputToTrackConnectionEvent::Action::Connect,
                                                            IMMEDIATE_PROCESS);

    auto event_status_connect = connection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect);

    // TODO/Q: This test fails when running tests in batch, but succeeds when run individually.
    // I introduced it, as I thought it was missing by omission.
    // But maybe there was a reason why sending midi out cannot be tested?
    //auto status2 = _midi_dispatcher.process(&event_ch5);
    //EXPECT_EQ(EventStatus::HANDLED_OK, status2);
    //EXPECT_TRUE(_test_frontend.midi_sent_on_input(0));

    auto disconnection_event = KbdOutputToTrackConnectionEvent(&_midi_dispatcher,
                                                               track_id,
                                                               channel,
                                                               port,
                                                               KbdOutputToTrackConnectionEvent::Action::Disconnect,
                                                               IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnection_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    auto status3 = _midi_dispatcher.process(&event_ch5);
    EXPECT_EQ(EventStatus::HANDLED_OK, status3);
    EXPECT_FALSE(_test_frontend.midi_sent_on_input(0));
}

TEST_F(MidiControllerEventTestFrontend, TestCCDataConnectionDisconnection)
{
    ext::MidiChannel channel = sushi::ext::MidiChannel::MIDI_CH_4;
    int port = 0;

    // The id for the mock processor is generated by a static atomic counter in BaseIdGenetator, so needs to be fetched.
    auto processor = _test_engine.processor_container()->processor("processor");
    ObjectId processor_id = processor->id();

    const std::string parameter_name = "param 1";

    _midi_dispatcher.set_midi_inputs(5);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 67:

    auto connect_event1 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    67, // cc_number
                                                   0, // min_range
                                                   100, // max_range
                                                   false, // use_relative_mode
                                                   IMMEDIATE_PROCESS);

    auto event_status_connect1 = connect_event1.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect1);

    // Connect CC Number 68:

    auto connect_event2 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    68, // cc_number
                                                    0, // min_range
                                                    100, // max_range
                                                    false, // use_relative_mode
                                                    IMMEDIATE_PROCESS);

    auto event_status_connect2 = connect_event2.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect2);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    // Connect CC Number 70:

    auto connect_event3 = ConnectCCToParameterEvent(&_midi_dispatcher,
                                                    processor_id,
                                                    parameter_name,
                                                    channel,
                                                    port,
                                                    70, // cc_number
                                                    0, // min_range
                                                    100, // max_range
                                                    false, // use_relative_mode
                                                    IMMEDIATE_PROCESS);

    auto event_status_connect3 = connect_event3.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect3);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect CC Number 67 only:

    auto disconnect_event = DisconnectCCEvent(&_midi_dispatcher,
                                              processor_id,
                                              channel,
                                              port,
                                              67, // cc_number
                                              IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnect_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_67, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_68, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_CTRL_CH_CH4_70, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all remaining CC's:

    auto disconnect_all_event = DisconnectAllCCFromProcessorEvent(&_midi_dispatcher,
                                                                  processor_id,
                                                                  IMMEDIATE_PROCESS);

    auto event_status_disconnect_all = disconnect_all_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect_all);

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

    auto connect_event1 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                                       sushi::ext::MidiChannel::MIDI_CH_5,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect1 = connect_event1.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect1);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 6:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto connect_event2 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                                       sushi::ext::MidiChannel::MIDI_CH_6,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect2 = connect_event2.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect2);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Connect Channel 7:

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    auto connect_event3 = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                       processor_id,
                                               sushi::ext::MidiChannel::MIDI_CH_7,
                                                       port,
                                                       PCToProcessorConnectionEvent::Action::Connect,
                                                       IMMEDIATE_PROCESS);

    auto event_status_connect3 = connect_event3.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_connect3);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect Channel 5 only:

    auto disconnect_event = PCToProcessorConnectionEvent(&_midi_dispatcher,
                                                         processor_id,
                                                         sushi::ext::MidiChannel::MIDI_CH_5,
                                                         port,
                                                         PCToProcessorConnectionEvent::Action::Disconnect,
                                                         IMMEDIATE_PROCESS);

    auto event_status_disconnect = disconnect_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_TRUE(_test_dispatcher->got_event());

    // Disconnect all channels:

    auto disconnect_all_event = DisconnectAllPCFromProcessorEvent(&_midi_dispatcher,
                                                                  processor_id,
                                                                  IMMEDIATE_PROCESS);

    auto event_status_disconnect_all = disconnect_all_event.execute(&_test_engine);
    ASSERT_EQ(EventStatus::HANDLED_OK, event_status_disconnect_all);

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH5, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH6, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());

    _midi_dispatcher.send_midi(port, TEST_PRG_CH_CH7, IMMEDIATE_PROCESS);
    EXPECT_FALSE(_test_dispatcher->got_event());
}
