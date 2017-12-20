#include "gtest/gtest.h"

#define private public

#include "library/rt_event_fifo.h"
#include "plugins/arpeggiator_plugin.cpp"

using namespace sushi;
using namespace sushi::arpeggiator_plugin;

class TestArpeggiator : public ::testing::Test
{
protected:
    TestArpeggiator() {}

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
    TestArpeggiatorPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new arpeggiator_plugin::ArpeggiatorPlugin();
        ProcessorReturnCode status = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_fifo);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    RtEventFifo _fifo;
    arpeggiator_plugin::ArpeggiatorPlugin* _module_under_test;
};


TEST_F(TestArpeggiatorPlugin, TestOutput)
{
    ChunkSampleBuffer buffer;
    auto event = RtEvent::make_note_on_event(0, 0, 50, 1.0f);
    _module_under_test->process_event(event);
    event = RtEvent::make_parameter_change_event(0, 0, 0, 120.0f);
    _module_under_test->process_event(event);

    /* 1/8 notes at 120 bpm equals 4 notes/sec, @48000 this means we need
     * tp process at least 12000 samples to catch one note */
    ASSERT_TRUE(_fifo.empty());
    for (int i = 0; i < 12500 / AUDIO_CHUNK_SIZE ; ++i)
    {
        _module_under_test->process_audio(buffer, buffer);
    }
    ASSERT_FALSE(_fifo.empty());
    RtEvent e;
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test->id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_OFF, e.type());
    _fifo.pop(e);
    ASSERT_EQ(_module_under_test->id(), e.processor_id());
    ASSERT_EQ(RtEventType::NOTE_ON, e.type());
    ASSERT_EQ(50, e.keyboard_event()->note());
}