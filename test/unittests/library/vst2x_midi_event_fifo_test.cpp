#include "gtest/gtest.h"

#include "library/vst2x_midi_event_fifo.h"

using namespace sushi;
using namespace sushi::vst2;

namespace {
    constexpr int TEST_FIFO_CAPACITY = 128;
    constexpr int TEST_DATA_SIZE = 100;
} // anonymous namespace

class TestVst2xMidiEventFIFO : public ::testing::Test
{
protected:
    TestVst2xMidiEventFIFO()
    {
    }

    void SetUp()
    {
        // Pre-fill queue
        for (int i=0; i<TEST_DATA_SIZE; i++)
        {
            auto ev = RtEvent::make_note_on_event(0, i, 0, 1.0f);
            ASSERT_EQ(true, _module_under_test.push(ev));
        }
    }

    void TearDown()
    {
    }

    Vst2xMidiEventFIFO<TEST_FIFO_CAPACITY> _module_under_test;
};

TEST_F(TestVst2xMidiEventFIFO, TestNonOverflowingBehaviour)
{

    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(i, midi_ev->deltaFrames);
    }
}

TEST_F(TestVst2xMidiEventFIFO, TestFlush)
{
    _module_under_test.flush();
    auto vst_events = _module_under_test.flush();
    EXPECT_EQ(0, vst_events->numEvents);
}

TEST_F(TestVst2xMidiEventFIFO, TestOverflow)
{
    constexpr int overflow_offset = 1000;

    for (int i=TEST_DATA_SIZE; i<TEST_FIFO_CAPACITY; i++)
    {
        auto ev = RtEvent::make_note_on_event(0, i, 0, 1.0f);
        ASSERT_TRUE(_module_under_test.push(ev));
    }
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto ev = RtEvent::make_note_on_event(0, overflow_offset+i, 0, 1.0f);
        ASSERT_FALSE(_module_under_test.push(ev));
    }
    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_FIFO_CAPACITY, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(overflow_offset + i, midi_ev->deltaFrames);
    }
}

TEST_F(TestVst2xMidiEventFIFO, TestFlushAfterOverflow)
{
    // Let the queue overflow...
    for (int i=0; i<(2*TEST_FIFO_CAPACITY); i++)
    {
        auto ev = RtEvent::make_note_on_event(0, i, 0, 1.0f);
        _module_under_test.push(ev);
    }
    _module_under_test.flush();

    // ... and check that after flushing is working again in normal, non-overflowed conditions
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto ev = RtEvent::make_note_on_event(0, i, 0, 1.0f);
        ASSERT_EQ(true, _module_under_test.push(ev));
    }
    auto vst_events = _module_under_test.flush();
    ASSERT_EQ(TEST_DATA_SIZE, vst_events->numEvents);
    for (int i=0; i<TEST_DATA_SIZE; i++)
    {
        auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[i]);
        EXPECT_EQ(i, midi_ev->deltaFrames);
    }
}

TEST_F(TestVst2xMidiEventFIFO, TestNoteOnCreation)
{
    _module_under_test.flush();
    auto ev = RtEvent::make_note_on_event(0, 0, 60, 1.0f);
    _module_under_test.push(ev);
    auto vst_events = _module_under_test.flush();
    auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[0]);

    EXPECT_EQ(static_cast<char>(144), midi_ev->midiData[0]);
    EXPECT_EQ(static_cast<char>(60), midi_ev->midiData[1]);
    EXPECT_EQ(static_cast<char>(127), midi_ev->midiData[2]);
}

TEST_F(TestVst2xMidiEventFIFO, TestNoteOffCreation)
{
    _module_under_test.flush();
    auto ev = RtEvent::make_note_off_event(0, 0, 72, 0.5f);
    _module_under_test.push(ev);
    auto vst_events = _module_under_test.flush();
    auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[0]);

    EXPECT_EQ(static_cast<char>(128), midi_ev->midiData[0]);
    EXPECT_EQ(static_cast<char>(72), midi_ev->midiData[1]);
    EXPECT_EQ(static_cast<char>(64), midi_ev->midiData[2]);
}

TEST_F(TestVst2xMidiEventFIFO, TestNoteAftertouchCreation)
{
    _module_under_test.flush();
    auto ev = RtEvent::make_note_aftertouch_event(0, 0, 127, 0.0f);
    _module_under_test.push(ev);
    auto vst_events = _module_under_test.flush();
    auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[0]);

    EXPECT_EQ(static_cast<char>(160), midi_ev->midiData[0]);
    EXPECT_EQ(static_cast<char>(127), midi_ev->midiData[1]);
    EXPECT_EQ(static_cast<char>(0), midi_ev->midiData[2]);
}

TEST_F(TestVst2xMidiEventFIFO, TestWrappedMidiCreation)
{
    _module_under_test.flush();
    auto ev = RtEvent::make_wrapped_midi_event(0, 0, {176u, 21u, 64u, 0u});
    _module_under_test.push(ev);
    auto vst_events = _module_under_test.flush();
    auto midi_ev = reinterpret_cast<VstMidiEvent*>(vst_events->events[0]);

    EXPECT_EQ(static_cast<char>(176), midi_ev->midiData[0]);
    EXPECT_EQ(static_cast<char>(21), midi_ev->midiData[1]);
    EXPECT_EQ(static_cast<char>(64), midi_ev->midiData[2]);
}



