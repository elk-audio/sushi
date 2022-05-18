#include "gtest/gtest.h"

#include "library/rt_event_fifo.h"
#include "plugins/step_sequencer_plugin.cpp"
#include "test_utils/host_control_mockup.h"

using namespace sushi;
using namespace sushi::step_sequencer_plugin;

constexpr float TEST_SAMPLERATE = 48000;

class TestStepSequencerPlugin : public ::testing::Test
{
protected:
    TestStepSequencerPlugin()
    {
    }
    void SetUp()
    {
        ProcessorReturnCode status = _module_under_test.init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test.set_event_output(&_fifo);
    }

    RtEventFifo<10> _fifo;
    HostControlMockup _host_control;
    StepSequencerPlugin _module_under_test{_host_control.make_host_control_mockup(TEST_SAMPLERATE)};
};

TEST_F(TestStepSequencerPlugin, TestOutput)
{
    ChunkSampleBuffer buffer;
    _host_control._transport.set_playing_mode(PlayingMode::PLAYING, false);
    _host_control._transport.set_tempo(120.0f, false);
    _host_control._transport.set_time(std::chrono::microseconds(0), 0);


    ASSERT_TRUE(_fifo.empty());
    /* 1/8 notes at 120 bpm equals 4 notes/sec, @48000 results having an
     * 8th note at 12000, so fast-forward the time so directly before this time */
    _host_control._transport.set_time(std::chrono::microseconds(249'500), 11'990);
    _module_under_test.process_audio(buffer, buffer);
    RtEvent e;
    bool got_event = _fifo.pop(e);
    ASSERT_TRUE(got_event);
    ASSERT_EQ(_module_under_test.id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_OFF, e.type());
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test.id(), e.processor_id());
    ASSERT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, e.type());
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test.id(), e.processor_id());
    ASSERT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, e.type());
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test.id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_ON, e.type());
    ASSERT_EQ(48, e.keyboard_event()->note());
    ASSERT_TRUE(_fifo.empty());
}
