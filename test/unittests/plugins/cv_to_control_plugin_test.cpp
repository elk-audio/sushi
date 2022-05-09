#include "gtest/gtest.h"

#include "plugins/cv_to_control_plugin.cpp"
#include "library/rt_event_fifo.h"
#include "test_utils/host_control_mockup.h"

using namespace sushi;
using namespace sushi::cv_to_control_plugin;

constexpr float TEST_SAMPLE_RATE = 44100;

TEST(CvToControlPluginTestExternalFunctions, TestCvToPitch)
{
    auto [note, fraction] = cv_to_pitch(0.502);
    EXPECT_EQ(60, note);
    EXPECT_FLOAT_EQ(0.23999786f, fraction);
}

class CvToControlPluginTest : public ::testing::Test
{
protected:
    CvToControlPluginTest() = default;

    void SetUp()
    {
        _module_under_test.init(TEST_SAMPLE_RATE);
        _module_under_test.set_event_output(&_event_output);
    }

    HostControlMockup _host_control;
    CvToControlPlugin _module_under_test{_host_control.make_host_control_mockup(TEST_SAMPLE_RATE)};
    RtEventFifo<10>   _event_output;
    ChunkSampleBuffer _audio_buffer{2};
};

TEST_F(CvToControlPluginTest, TestMonophonicMode)
{
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_TRUE(_event_output.empty());

    // Set pitch parameter and send a gate high event, we should receive a note on event
    auto event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("polyphony")->id(), 0);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.5);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    auto recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_EQ(60, recv_event.keyboard_event()->note());
    EXPECT_FLOAT_EQ(1.0f, recv_event.keyboard_event()->velocity());
    EXPECT_TRUE(_event_output.empty());

    // Change the pitch enough to trigger a new note on
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.51);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_EQ(61, recv_event.keyboard_event()->note());
    EXPECT_TRUE(_event_output.empty());

    // The note off should come the next buffer, to enable soft synths to do legato
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_OFF, recv_event.type());
    EXPECT_EQ(60, recv_event.keyboard_event()->note());

    // Now Send a gate low event, and we should receive a note off with the new note
    event = RtEvent::make_note_off_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_OFF, recv_event.type());
    EXPECT_EQ(61, recv_event.keyboard_event()->note());
    EXPECT_TRUE(_event_output.empty());
}

TEST_F(CvToControlPluginTest, TestPitchBendMode)
{
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_TRUE(_event_output.empty());

    // Set pitch parameter and send a gate high event, we should receive a note on event and a pitch bend
    auto event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("polyphony")->id(), 0);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_bend_enabled")->id(), 1);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.501);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    auto recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_EQ(60, recv_event.keyboard_event()->note());
    EXPECT_FLOAT_EQ(1.0f, recv_event.keyboard_event()->velocity());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::PITCH_BEND, recv_event.type());
    float initial_pb = recv_event.keyboard_common_event()->value();
    EXPECT_GT(initial_pb, 0.0f);
    EXPECT_TRUE(_event_output.empty());

    // Change the pitch up ~one semitone, this should not send a new note on, only a pitch bend with a higher value
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.51);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::PITCH_BEND, recv_event.type());
    EXPECT_GT(recv_event.keyboard_common_event()->value() , initial_pb);
    EXPECT_TRUE(_event_output.empty());
}

TEST_F(CvToControlPluginTest, TestVelocity)
{
    // Set pitch parameter and send a gate high event, we should receive a note on event and a pitch bend
    auto event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("velocity_enabled")->id(), 1);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.5);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("velocity_0")->id(), 0.75);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    auto recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_EQ(60, recv_event.keyboard_event()->note());
    EXPECT_FLOAT_EQ(0.75f, recv_event.keyboard_event()->velocity());
    EXPECT_TRUE(_event_output.empty());
}

TEST_F(CvToControlPluginTest, TestPolyphony)
{
    // Set pitch parameter and send a gate high event, we should receive a note on event
    auto event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("polyphony")->id(), 4);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_0")->id(), 0.5);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    EXPECT_FALSE(_event_output.empty());
    auto recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_EQ(60, recv_event.keyboard_event()->note());
    EXPECT_FLOAT_EQ(1.0f, recv_event.keyboard_event()->velocity());
    EXPECT_TRUE(_event_output.empty());

    // Sent 2 new gate ons
    event = RtEvent::make_note_on_event(0, 0, 0, 1, 1.0f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(0, 0, 0, 2, 1.0f);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, _module_under_test.parameter_from_name("pitch_1")->id(), 0.3);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_ON, recv_event.type());
    EXPECT_TRUE(_event_output.empty());

    // Sent 2 new gate offs
    event = RtEvent::make_note_off_event(0, 0, 0, 0, 1.0f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_off_event(0, 0, 0, 2, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_OFF, recv_event.type());
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_OFF, recv_event.type());
    EXPECT_TRUE(_event_output.empty());

    event = RtEvent::make_note_off_event(0, 0, 0, 1, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    recv_event = _event_output.pop();
    EXPECT_EQ(RtEventType::NOTE_OFF, recv_event.type());
    EXPECT_TRUE(_event_output.empty());
}