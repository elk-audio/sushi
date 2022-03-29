#include "gtest/gtest.h"

#include "library/rt_event_fifo.h"
#include "test_utils/host_control_mockup.h"

#define private public

#include "plugins/arpeggiator_plugin.cpp"

using namespace sushi;
using namespace sushi::arpeggiator_plugin;

constexpr float TEST_SAMPLERATE = 48000;

class TestArpeggiator : public ::testing::Test
{
protected:
    TestArpeggiator() = default;

    void SetUp()
    {}

    Arpeggiator _module_under_test;
};


TEST_F(TestArpeggiator, TestOperation)
{
    _module_under_test.add_note(10);
    _module_under_test.add_note(14);
    _module_under_test.add_note(17);
    _module_under_test.set_range(2);

    /* Play chord in 2 octaves */
    EXPECT_EQ(10, _module_under_test.next_note());
    EXPECT_EQ(14, _module_under_test.next_note());
    EXPECT_EQ(17, _module_under_test.next_note());
    EXPECT_EQ(22, _module_under_test.next_note());
    EXPECT_EQ(26, _module_under_test.next_note());
    EXPECT_EQ(29, _module_under_test.next_note());
    EXPECT_EQ(10, _module_under_test.next_note());

    _module_under_test.remove_note(14);
    _module_under_test.set_range(1);

    EXPECT_EQ(17, _module_under_test.next_note());
    EXPECT_EQ(10, _module_under_test.next_note());
}


TEST_F(TestArpeggiator, TestHold)
{
    _module_under_test.set_range(2);
    _module_under_test.add_note(15);
    _module_under_test.remove_note(15);

    EXPECT_EQ(15, _module_under_test.next_note());
    EXPECT_EQ(27, _module_under_test.next_note());
    EXPECT_EQ(15, _module_under_test.next_note());

    _module_under_test.add_note(14);
    _module_under_test.add_note(17);

    EXPECT_EQ(17, _module_under_test.next_note());
    EXPECT_EQ(26, _module_under_test.next_note());
    EXPECT_EQ(29, _module_under_test.next_note());
    EXPECT_EQ(14, _module_under_test.next_note());

    _module_under_test.remove_note(17);
    _module_under_test.remove_note(14);

    EXPECT_EQ(26, _module_under_test.next_note());
    EXPECT_EQ(14, _module_under_test.next_note());
}


class TestArpeggiatorPlugin : public ::testing::Test
{
protected:
    TestArpeggiatorPlugin() = default;

    void SetUp()
    {
        _module_under_test = new arpeggiator_plugin::ArpeggiatorPlugin(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_fifo);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    RtSafeRtEventFifo _fifo;
    HostControlMockup _host_control;
    arpeggiator_plugin::ArpeggiatorPlugin* _module_under_test;
};


TEST_F(TestArpeggiatorPlugin, TestOutput)
{
    ChunkSampleBuffer buffer;
    auto event = RtEvent::make_note_on_event(0, 0, 0, 50, 1.0f);
    _module_under_test->process_event(event);
    _host_control._transport.set_tempo(120.0f, false);
    _host_control._transport.set_playing_mode(PlayingMode::PLAYING, false);
    _host_control._transport.set_time(std::chrono::milliseconds(0), 0);

    ASSERT_TRUE(_fifo.empty());
    /* 1/8 notes at 120 bpm equals 4 notes/sec, @48000 results in  at least
     * 12000 samples and 1/4 sec to catch one note, use the host control to
     * fast forward the time directly */
    _host_control._transport.set_time(std::chrono::milliseconds(250), 12000);
    _module_under_test->process_audio(buffer, buffer);
    RtEvent e;
    bool got_event = _fifo.pop(e);
    ASSERT_TRUE(got_event);
    ASSERT_EQ(_module_under_test->id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_OFF, e.type());
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test->id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_ON, e.type());
    ASSERT_EQ(50, e.keyboard_event()->note());
    ASSERT_TRUE(_fifo.empty());
}
