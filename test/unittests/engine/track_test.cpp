#include "gtest/gtest.h"

#include "engine/transport.h"

#define private public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "engine/track.cpp"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"

using namespace sushi;
using namespace engine;

class DummyProcessor : public Processor
{
public:
    DummyProcessor(HostControl host_control) : Processor(host_control)
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
    }

    ProcessorReturnCode init(float /* sample_rate */) override
    {
        return ProcessorReturnCode::OK;
    }

    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
    {
        out_buffer = in_buffer;
    }
};

class DummyMonoProcessor : public DummyProcessor
{
public:
    DummyMonoProcessor(HostControl host_control) :DummyProcessor(host_control)
    {
        _max_input_channels = 1;
        _max_output_channels = 1;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
    }
};

class TrackTest : public ::testing::Test
{
protected:
    TrackTest() {}

    HostControlMockup _host_control;
    performance::ProcessTimer _timer;
    Track _module_under_test{_host_control.make_host_control_mockup(), 2, &_timer};
};


TEST_F(TrackTest, TestChannelManagement)
{
    DummyProcessor test_processor(_host_control.make_host_control_mockup());
    test_processor.set_input_channels(2);
    /* Add the test processor to a mono track and verify
     * it is configured in mono config */
    Track _module_under_test_mono(_host_control.make_host_control_mockup(), 1, &_timer);
    _module_under_test_mono.set_input_channels(1);
    _module_under_test_mono.add(&test_processor);
    EXPECT_EQ(1, test_processor.input_channels());
    EXPECT_EQ(1, test_processor.output_channels());

    /* Put a stereo and then a mono-only plugin on a
     * stereo track */
    gain_plugin::GainPlugin gain_plugin(_host_control.make_host_control_mockup());
    DummyMonoProcessor mono_processor(_host_control.make_host_control_mockup());
    _module_under_test.set_output_channels(1);
    _module_under_test.add(&gain_plugin);
    _module_under_test.add(&mono_processor);

    EXPECT_EQ(2, _module_under_test.input_channels());
    EXPECT_EQ(1, _module_under_test.output_channels());
    EXPECT_EQ(2, gain_plugin.input_channels());
    EXPECT_EQ(1, gain_plugin.output_channels());
    EXPECT_EQ(1, mono_processor.input_channels());
    EXPECT_EQ(1, mono_processor.output_channels());

    /* Set the input to mono and watch the plugins adapt */
    _module_under_test.set_input_channels(1);
    EXPECT_EQ(1, _module_under_test.input_channels());
    EXPECT_EQ(1, gain_plugin.input_channels());
    EXPECT_EQ(1, gain_plugin.output_channels());
}

TEST_F(TrackTest, TestMultibusSetup)
{
    Track module_under_test((_host_control.make_host_control_mockup()), 2, 2, &_timer);
    EXPECT_EQ(2, module_under_test.input_busses());
    EXPECT_EQ(2, module_under_test.output_busses());
    EXPECT_EQ(4, module_under_test.parameter_count());
    EXPECT_EQ(2, module_under_test.input_bus(1).channel_count());
    EXPECT_EQ(2, module_under_test.output_bus(1).channel_count());
}

TEST_F(TrackTest, TestAddAndRemove)
{
    DummyProcessor test_processor(_host_control.make_host_control_mockup());
    _module_under_test.add(&test_processor);
    EXPECT_EQ(1u, _module_under_test._processors.size());
    EXPECT_FALSE(_module_under_test.remove(1234567u));
    EXPECT_EQ(1u, _module_under_test._processors.size());
    EXPECT_TRUE(_module_under_test.remove(test_processor.id()));
    EXPECT_TRUE(_module_under_test._processors.empty());
}

TEST_F(TrackTest, TestNestedBypass)
{
    DummyProcessor test_processor(_host_control.make_host_control_mockup());
    _module_under_test.add(&test_processor);
    _module_under_test.set_bypassed(true);
    EXPECT_TRUE(test_processor.bypassed());
}

TEST_F(TrackTest, TestEmptyChainRendering)
{
    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);
    test_utils::assert_buffer_value(1.0f, out);
}

TEST_F(TrackTest, TestRenderingWithProcessors)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    _module_under_test.add(&plugin);

    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);
    test_utils::assert_buffer_value(1.0f, out);
}

TEST_F(TrackTest, TestPanAndGain)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    _module_under_test.add(&plugin);
    auto gain_param = _module_under_test.parameter_from_name("gain");
    auto pan_param = _module_under_test.parameter_from_name("pan");
    ASSERT_FALSE(gain_param == nullptr);
    ASSERT_FALSE(pan_param == nullptr);

    /* Pan hard right and volume up 6 dB */
    auto gain_ev = RtEvent::make_parameter_change_event(0, 0, gain_param->id(), 6.0f);
    auto pan_ev = RtEvent::make_parameter_change_event(0, 0, pan_param->id(), 1.0f);

    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.process_event(gain_ev);
    _module_under_test.process_event(pan_ev);

    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);

    /* Exact values will be tested by the pan function, just verify that it had an effect */
    EXPECT_EQ(0.0f, out.channel(LEFT_CHANNEL_INDEX)[0]);
    EXPECT_LT(2.0f, out.channel(RIGHT_CHANNEL_INDEX)[0]);
}

TEST_F(TrackTest, TestEventProcessing)
{
    ChunkSampleBuffer buffer(2);
    RtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_event_output(&event_queue);
    _module_under_test.set_input_channels(2);
    _module_under_test.set_output_channels(2);
    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    RtEvent event = RtEvent::make_note_on_event(0, 0, 0, 0);

    _module_under_test.process_event(event);
    _module_under_test.render();
    ASSERT_FALSE(event_queue.empty());
    RtEvent e;
    event_queue.pop(e);

    /* Test with internal event buffering */
    _module_under_test.set_event_output_internal();
    auto& output_event_buffer = _module_under_test.output_event_buffer();
    ASSERT_TRUE(output_event_buffer.empty());

    _module_under_test.process_event(event);
    _module_under_test.render();
    ASSERT_FALSE(output_event_buffer.empty());
    ASSERT_TRUE(event_queue.empty());

}

TEST_F(TrackTest, TestEventForwarding)
{
    ChunkSampleBuffer buffer(2);
    RtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_event_output(&event_queue);

    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    RtEvent event = RtEvent::make_note_on_event(125, 13, 48, 0.0f);

    _module_under_test.process_event(event);
    _module_under_test.process_audio(buffer, buffer);
    ASSERT_FALSE(event_queue.empty());
    RtEvent received_event;
    bool got_event = event_queue.pop(received_event);
    ASSERT_TRUE(got_event);
    auto typed_event = received_event.keyboard_event();
    ASSERT_EQ(RtEventType::NOTE_ON, received_event.type());
    /* Assert that the processor id of the event is that of the track and
     * not the id set. */
    ASSERT_EQ(_module_under_test.id(), typed_event->processor_id());
}

TEST(TestStandAloneFunctions, TestApplyPanAndGain)
{
    ChunkSampleBuffer buffer(2);
    test_utils::fill_sample_buffer(buffer, 2.0f);
    apply_pan_and_gain(buffer, 5.0f, 0);
    test_utils::assert_buffer_value(10.0f, buffer);

    /* Pan hard right */
    test_utils::fill_sample_buffer(buffer, 1.0f);
    apply_pan_and_gain(buffer, 1.0f, 1.0f);
    EXPECT_EQ(0.0f, buffer.channel(LEFT_CHANNEL_INDEX)[0]);
    EXPECT_NEAR(1.41f, buffer.channel(RIGHT_CHANNEL_INDEX)[0], 0.01f);

    /* Pan mid left */
    test_utils::fill_sample_buffer(buffer, 1.0f);
    apply_pan_and_gain(buffer, 1.0f, -0.5f);
    EXPECT_NEAR(1.2f, buffer.channel(LEFT_CHANNEL_INDEX)[0], 0.01f);
    EXPECT_EQ(0.5f, buffer.channel(RIGHT_CHANNEL_INDEX)[0]);
}