#include "gtest/gtest.h"

#include "plugins/control_to_cv_plugin.cpp"
#include "library/rt_event_fifo.h"
#include "test_utils/host_control_mockup.h"

#include <iostream>

using namespace sushi;
using namespace sushi::control_to_cv_plugin;

constexpr float TEST_SAMPLE_RATE = 44100;

TEST(ControlToCvPluginTestExternalFunctions, TestPitchToCv)
{
    EXPECT_FLOAT_EQ(0.5f, pitch_to_cv(60.0f));
    // TODO - Add more test when we have a proper midi note to scaling function in place
}

class ControlToCvPluginTest : public ::testing::Test
{
protected:
    ControlToCvPluginTest() = default;

    void SetUp()
    {
        _module_under_test.init(TEST_SAMPLE_RATE);
        _module_under_test.set_event_output(&_event_output);
    }

    HostControlMockup _host_control;
    ControlToCvPlugin _module_under_test{_host_control.make_host_control_mockup(TEST_SAMPLE_RATE)};
    RtEventFifo<10>   _event_output;
    ChunkSampleBuffer _audio_buffer{2};
};

TEST_F(ControlToCvPluginTest, TestMonophonicMode)
{
    constexpr int PITCH_CV = 1;

    // Only connect 1 pitch output
    auto status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("pitch_0")->id(), PITCH_CV);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_gate_from_processor(0, 0, 0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    ASSERT_EQ(1, _event_output.size());
    EXPECT_EQ(RtEventType::CV_EVENT, _event_output[0].type());
    _event_output.clear();

    // Send a note on message
    auto event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 60, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // We should receive 1 gate high and 1 pitch CV event
    ASSERT_EQ(2, _event_output.size());
    EXPECT_EQ(RtEventType::GATE_EVENT, _event_output[0].type());
    EXPECT_TRUE(_event_output[0].gate_event()->value());
    EXPECT_EQ(0, _event_output[0].gate_event()->gate_no());

    EXPECT_EQ(RtEventType::CV_EVENT, _event_output[1].type());
    EXPECT_EQ(PITCH_CV, _event_output[1].cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.5f, _event_output[1].cv_event()->value());
    _event_output.clear();

    // Send another note on message
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 48, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    // We should receive 1 pitch CV event with lower pitch
    ASSERT_EQ(1, _event_output.size());
    EXPECT_EQ(RtEventType::CV_EVENT, _event_output[0].type());
    EXPECT_EQ(PITCH_CV, _event_output[0].cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.4f, _event_output[0].cv_event()->value());
    _event_output.clear();

    // Enable retrigger and send yet another note on
    event = RtEvent::make_parameter_change_event(_module_under_test.id(), 0, _module_under_test.parameter_from_name("retrigger_enabled")->id(), 1.0f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 66, 1.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // We should receive 1 gate low and 1 pitch CV event
    ASSERT_EQ(2, _event_output.size());
    EXPECT_EQ(RtEventType::GATE_EVENT, _event_output[0].type());
    EXPECT_FALSE(_event_output[0].gate_event()->value());
    EXPECT_EQ(0, _event_output[0].gate_event()->gate_no());

    EXPECT_EQ(RtEventType::CV_EVENT, _event_output[1].type());
    EXPECT_EQ(PITCH_CV, _event_output[1].cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.55f, _event_output[1].cv_event()->value());
    _event_output.clear();

    // And the gate high should come the next buffer
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);
    ASSERT_EQ(2, _event_output.size());
    EXPECT_EQ(RtEventType::GATE_EVENT, _event_output[0].type());
    EXPECT_TRUE(_event_output[0].gate_event()->value());
    EXPECT_EQ(0, _event_output[0].gate_event()->gate_no());

    EXPECT_EQ(RtEventType::CV_EVENT, _event_output[1].type());
    EXPECT_EQ(PITCH_CV, _event_output[1].cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.55f, _event_output[1].cv_event()->value());
    _event_output.clear();
}

TEST_F(ControlToCvPluginTest, TestPolyphonicMode)
{
    constexpr int PITCH_CV_1 = 0;
    constexpr int PITCH_CV_2 = 1;
    constexpr int VEL_CV_1 = 2;
    constexpr int VEL_CV_2 = 3;

    // Use 2 pitch and 2 velocity outputs
    auto status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("pitch_0")->id(), PITCH_CV_1);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("pitch_1")->id(), PITCH_CV_2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("velocity_0")->id(), VEL_CV_1);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("velocity_1")->id(), VEL_CV_2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_gate_from_processor(0, 0, 0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_gate_from_processor(1, 0, 1);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    auto event = RtEvent::make_parameter_change_event(_module_under_test.id(), 0, _module_under_test.parameter_from_name("polyphony")->id(), 2.0f);
    _module_under_test.process_event(event);
    event = RtEvent::make_parameter_change_event(_module_under_test.id(), 0, _module_under_test.parameter_from_name("send_velocity")->id(), 1.0f);
    _module_under_test.process_event(event);

    // Send 2 note on messages
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 60, 0.75f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 48, 0.5f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    ASSERT_EQ(6, _event_output.size());
    auto e = _event_output.pop();
    EXPECT_EQ(RtEventType::GATE_EVENT, e.type());
    EXPECT_TRUE(e.gate_event()->value());
    EXPECT_EQ(0, e.gate_event()->gate_no());

    e = _event_output.pop();
    EXPECT_TRUE(e.gate_event()->value());
    EXPECT_EQ(1, e.gate_event()->gate_no());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV_1, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.50, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV_2, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.40, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(VEL_CV_1, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.75, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(VEL_CV_2, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.50, e.cv_event()->value());

    _event_output.clear();

    // Send 1 more note on message
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 78, 0.4f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // Should not result in any more gate low events, but pitch cv 1 should change.
    ASSERT_EQ(4, _event_output.size());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV_1, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.65, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV_2, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.4, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(VEL_CV_1, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.4, e.cv_event()->value());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(VEL_CV_2, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.5, e.cv_event()->value());

    _event_output.clear();

    // Send 3 note off messages
    event = RtEvent::make_note_off_event(_module_under_test.id(), 0, 0, 78, 0.5f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_off_event(_module_under_test.id(), 0, 0, 48, 0.5f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_off_event(_module_under_test.id(), 0, 0, 60, 0.5f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    ASSERT_EQ(6, _event_output.size());
    e = _event_output.pop();
    EXPECT_EQ(RtEventType::GATE_EVENT, e.type());
    EXPECT_FALSE(e.gate_event()->value());
    EXPECT_EQ(0, e.gate_event()->gate_no());

    e = _event_output.pop();
    EXPECT_EQ(RtEventType::GATE_EVENT, e.type());
    EXPECT_FALSE(e.gate_event()->value());
    EXPECT_EQ(1, e.gate_event()->gate_no());
    _event_output.clear();
}

TEST_F(ControlToCvPluginTest, TestPitchBend)
{
    constexpr int PITCH_CV = 2;
    auto status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("pitch_0")->id(), PITCH_CV);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_gate_from_processor(0, 0, 0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Send a note on message and a pitch bend message
    auto event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 48, 0.5f);
    _module_under_test.process_event(event);
    event = RtEvent::make_pitch_bend_event(_module_under_test.id(), 0, 0, 0.5f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // We should receive 1 events, gate and pitch,
    ASSERT_EQ(2, _event_output.size());
    auto e = _event_output[1];
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.45, e.cv_event()->value());
    _event_output.clear();

    // Set the tune parameters up 1 octave
    event = RtEvent::make_parameter_change_event(_module_under_test.id(), 0, _module_under_test.parameter_from_name("tune")->id(), 12.0f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // We should receive 1 pitch event,
    ASSERT_EQ(1, _event_output.size());
    e = _event_output[0];
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(PITCH_CV, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.55, e.cv_event()->value());
    _event_output.clear();
}

TEST_F(ControlToCvPluginTest, TestModulation)
{
    constexpr int PITCH_CV = 0;
    constexpr int MOD_CV = 1;
    // Use 2 pitch and 2 velocity outputs
    auto status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("modulation")->id(), MOD_CV);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_cv_from_parameter(_module_under_test.parameter_from_name("pitch_0")->id(), PITCH_CV);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    status = _module_under_test.connect_gate_from_processor(0, 0, 0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    auto event = RtEvent::make_parameter_change_event(_module_under_test.id(), 0, _module_under_test.parameter_from_name("send_modulation")->id(), 2.0f);
    _module_under_test.process_event(event);

    // Send 2 note on messages
    event = RtEvent::make_kb_modulation_event(_module_under_test.id(), 0, 0, 0.5f);
    _module_under_test.process_event(event);
    event = RtEvent::make_note_on_event(_module_under_test.id(), 0, 0, 48, 0.1f);
    _module_under_test.process_event(event);
    _module_under_test.process_audio(_audio_buffer, _audio_buffer);

    // We should receive 3 events, gate, pitch and modulation cv
    ASSERT_EQ(3, _event_output.size());

    auto e = _event_output[2];
    EXPECT_EQ(RtEventType::CV_EVENT, e.type());
    EXPECT_EQ(MOD_CV, e.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.5, e.cv_event()->value());
}

