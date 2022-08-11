#include "gtest/gtest.h"

#define private public
#include "engine/track.cpp"
#undef private

#include "engine/transport.h"
#include "plugins/passthrough_plugin.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "test_utils/dummy_processor.h"

using namespace sushi;
using namespace engine;

constexpr float TEST_SAMPLE_RATE = 48000;
constexpr int TEST_CHANNEL_COUNT = 2;
constexpr bool CREATE_PAN_CONTROLS = true;
class TrackTest : public ::testing::Test
{
protected:
    TrackTest() {}

    void SetUp()
    {
        _module_under_test.init(TEST_SAMPLE_RATE);
    }
    HostControlMockup _host_control;
    performance::PerformanceTimer _timer;
    Track _module_under_test{_host_control.make_host_control_mockup(), TEST_CHANNEL_COUNT, &_timer, CREATE_PAN_CONTROLS};
};

TEST_F(TrackTest, TestMultibusSetup)
{
    Track module_under_test((_host_control.make_host_control_mockup()), 2, &_timer);
    EXPECT_EQ(2, module_under_test.buses());
    EXPECT_EQ(5, module_under_test.parameter_count());
    EXPECT_EQ(2, module_under_test.input_bus(1).channel_count());
    EXPECT_EQ(2, module_under_test.output_bus(1).channel_count());
}

TEST_F(TrackTest, TestAddAndRemove)
{
    DummyProcessor test_processor(_host_control.make_host_control_mockup());
    DummyProcessor test_processor_2(_host_control.make_host_control_mockup());

    // Add to back
    auto ok = _module_under_test.add(&test_processor);
    EXPECT_TRUE(ok);
    EXPECT_EQ(1u, _module_under_test._processors.size());
    EXPECT_FALSE(_module_under_test.remove(1234567u));
    EXPECT_EQ(1u, _module_under_test._processors.size());

    // Add test_processor_2 to the front
    ok = _module_under_test.add(&test_processor_2, test_processor.id());
    EXPECT_TRUE(ok);
    EXPECT_EQ(2u, _module_under_test._processors.size());
    EXPECT_EQ(&test_processor_2, _module_under_test._processors[0]);
    EXPECT_EQ(&test_processor, _module_under_test._processors[1]);

    EXPECT_TRUE(_module_under_test.remove(test_processor.id()));
    EXPECT_TRUE(_module_under_test.remove(test_processor_2.id()));
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
    test_utils::assert_buffer_value(1.0f, out, test_utils::DECIBEL_ERROR);
}

TEST_F(TrackTest, TestRenderingWithProcessors)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(TEST_CHANNEL_COUNT);
    plugin.set_output_channels(TEST_CHANNEL_COUNT);

    _module_under_test.add(&plugin);

    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);
    test_utils::assert_buffer_value(1.0f, out, test_utils::DECIBEL_ERROR);
}

TEST_F(TrackTest, TestPanAndGain)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(TEST_CHANNEL_COUNT);
    plugin.set_output_channels(TEST_CHANNEL_COUNT);

    _module_under_test.add(&plugin);
    auto gain_param = _module_under_test.parameter_from_name("gain");
    auto pan_param = _module_under_test.parameter_from_name("pan");
    ASSERT_FALSE(gain_param == nullptr);
    ASSERT_FALSE(pan_param == nullptr);

    /* Pan hard right and volume up 6 dB */
    auto gain_ev = RtEvent::make_parameter_change_event(0, 0, gain_param->id(), 0.875);
    auto pan_ev = RtEvent::make_parameter_change_event(0, 0, pan_param->id(), 1.0f);

    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.process_event(gain_ev);
    _module_under_test.process_event(pan_ev);

    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);

    /* As volume changes will be smoothed, we won't get the exact result. Just verify
     * that it had an effect. Exact values will be tested by the pan function */
    EXPECT_LT(out.channel(LEFT_CHANNEL_INDEX)[AUDIO_CHUNK_SIZE-1], 1.0f);
    EXPECT_GT(out.channel(RIGHT_CHANNEL_INDEX)[AUDIO_CHUNK_SIZE-1], 1.0f);
}

TEST_F(TrackTest, TestPanAndGainPerBus)
{
    Track multibus_track((_host_control.make_host_control_mockup()), 2, &_timer);
    multibus_track.init(TEST_SAMPLE_RATE);

    auto gain_bus_0 = multibus_track.parameter_from_name("gain");
    auto gain_bus_1 = multibus_track.parameter_from_name("gain_sub_1");
    auto pan_bus_0 = multibus_track.parameter_from_name("pan");
    auto pan_bus_1 = multibus_track.parameter_from_name("pan_sub_1");
    ASSERT_TRUE(gain_bus_0);
    ASSERT_TRUE(gain_bus_1);
    ASSERT_TRUE(pan_bus_0);
    ASSERT_TRUE(pan_bus_1);

    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(multibus_track.input_channels());
    plugin.set_output_channels(multibus_track.input_channels());

    multibus_track.add(&plugin);

    /* Pan hard right/left and volume up/down 6 dB */
    auto gain_ev_0 = RtEvent::make_parameter_change_event(0, 0, gain_bus_0->id(), 0.875);
    auto gain_ev_1 = RtEvent::make_parameter_change_event(0, 0, gain_bus_1->id(), 0.875);
    auto pan_ev_0 = RtEvent::make_parameter_change_event(0, 0, pan_bus_0->id(), 1.0f);
    auto pan_ev_1 = RtEvent::make_parameter_change_event(0, 0, pan_bus_1->id(), 0.0f);

    auto in_bus = multibus_track.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    in_bus = multibus_track.input_bus(1);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    multibus_track.process_event(gain_ev_0);
    multibus_track.process_event(gain_ev_1);
    multibus_track.process_event(pan_ev_0);
    multibus_track.process_event(pan_ev_1);

    multibus_track.render();
    auto out = multibus_track.output_bus(0);

    /* As volume changes will be smoothed, we won't get the exact result. Just verify
     * that it had an effect. Exact values will be tested by the pan function */
    EXPECT_LT(out.channel(LEFT_CHANNEL_INDEX)[AUDIO_CHUNK_SIZE-1], 1.0f);
    EXPECT_GT(out.channel(RIGHT_CHANNEL_INDEX)[AUDIO_CHUNK_SIZE-1], 1.0f);
    EXPECT_GT(out.channel(2)[AUDIO_CHUNK_SIZE-1], 1.0f);
    EXPECT_LT(out.channel(3)[AUDIO_CHUNK_SIZE-1], 1.0f);
}

TEST_F(TrackTest, TestGainOnly)
{
    Track gain_only_track((_host_control.make_host_control_mockup()), 4, &_timer, false);
    gain_only_track.init(TEST_SAMPLE_RATE);

    auto gain_bus_0 = gain_only_track.parameter_from_name("gain");
    EXPECT_FALSE(gain_only_track.parameter_from_name("pan"));
    ASSERT_TRUE(gain_bus_0);

    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(gain_only_track.input_channels());
    plugin.set_output_channels(gain_only_track.input_channels());

    gain_only_track.add(&plugin);

    /* Volume down 6 dB */
    auto gain_ev_0 = RtEvent::make_parameter_change_event(0, 0, gain_bus_0->id(), 0.7917);
    gain_only_track.process_event(gain_ev_0);

    for (int i = 0; i < gain_only_track.max_input_channels(); ++i)
    {
        auto in_bus = gain_only_track.input_channel(i);
        test_utils::fill_sample_buffer(in_bus, 1.0f);
    }

    gain_only_track.render();
    auto out = gain_only_track.output_bus(0);

    /* As volume changes will be smoothed, we won't get the exact result. Just verify
     * that it had an effect. Exact values will be tested by the pan function */
    for (int i = 0; i < gain_only_track.max_output_channels(); ++i)
    {
        auto in_bus = gain_only_track.output_channel(i);
        EXPECT_LT(out.channel(0)[AUDIO_CHUNK_SIZE-1], 1.0f);
    }
}

TEST_F(TrackTest, TestMute)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(TEST_CHANNEL_COUNT);
    plugin.set_output_channels(TEST_CHANNEL_COUNT);

    _module_under_test.add(&plugin);
    auto mute_param = _module_under_test.parameter_from_name("mute");
    ASSERT_FALSE(mute_param == nullptr);

    // Mute should be off by default
    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);
    _module_under_test.render();
    test_utils::assert_buffer_value(1.0f, _module_under_test.output_bus(0));

    // Enable mute and run
    auto mute_event = RtEvent::make_parameter_change_event(0, 0, mute_param->id(), 1.0);
    _module_under_test.process_event(mute_event);
    for (int i = 0; i <= TEST_SAMPLE_RATE / AUDIO_CHUNK_SIZE / (1000 / GAIN_SMOOTHING_TIME.count()); ++i)
    {
        test_utils::fill_sample_buffer(in_bus, 1.0f);
        _module_under_test.render();
        EXPECT_LT(_module_under_test.output_bus(0).channel(0)[AUDIO_CHUNK_SIZE - 1], 1.0f);
    }
    EXPECT_LT(_module_under_test.output_bus(0).channel(0)[AUDIO_CHUNK_SIZE - 1], 0.1f);
}

TEST_F(TrackTest, TestEventProcessing)
{
    ChunkSampleBuffer buffer(2);
    RtSafeRtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(TEST_CHANNEL_COUNT);
    plugin.set_output_channels(TEST_CHANNEL_COUNT);

    plugin.set_event_output(&event_queue);
    _module_under_test.set_input_channels(2);
    _module_under_test.set_output_channels(2);
    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    RtEvent event = RtEvent::make_note_on_event(0, 0, 0, 0, 0);

    _module_under_test.process_event(event);
    _module_under_test.render();
    ASSERT_FALSE(event_queue.empty());
    RtEvent e;
    event_queue.pop(e);
}

TEST_F(TrackTest, TestEventForwarding)
{
    ChunkSampleBuffer buffer(2);
    RtSafeRtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(TEST_CHANNEL_COUNT);
    plugin.set_output_channels(TEST_CHANNEL_COUNT);
    plugin.set_event_output(&event_queue);

    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    RtEvent event = RtEvent::make_note_on_event(125, 13, 0, 48, 0.0f);

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

TEST_F(TrackTest, TestSilenceUnusedChannels)
{
    passthrough_plugin::PassthroughPlugin plugin(_host_control.make_host_control_mockup());
    plugin.init(44100);
    plugin.set_enabled(true);
    plugin.set_input_channels(1);
    plugin.set_output_channels(1);

    // Add a mono plugin to a stereo track.
    _module_under_test.add(&plugin);

    // Put some noise in the input buffer
    auto in_bus = _module_under_test.input_bus(0);
    test_utils::fill_sample_buffer(in_bus, 1.0f);

    _module_under_test.render();
    auto out = _module_under_test.output_bus(0);

    auto left_channel = ChunkSampleBuffer::create_non_owning_buffer(out, LEFT_CHANNEL_INDEX, 1);
    auto right_channel = ChunkSampleBuffer::create_non_owning_buffer(out, RIGHT_CHANNEL_INDEX, 1);
    test_utils::assert_buffer_value(1.0f, left_channel);
    test_utils::assert_buffer_value(0.0f, right_channel);
}

TEST(TestStandAloneFunctions, TesPanAndGainCalculation)
{
    auto [left_gain, right_gain] = calc_l_r_gain(5.0f, 0.0f);
    EXPECT_FLOAT_EQ(5, left_gain);
    EXPECT_FLOAT_EQ(5, right_gain);

    /* Pan hard right */
    std::tie(left_gain, right_gain) = calc_l_r_gain(1.0f, 1.0f);
    EXPECT_FLOAT_EQ(0.0f, left_gain);
    EXPECT_NEAR(1.41, right_gain, 0.01f);

    /* Pan mid left */
    std::tie(left_gain, right_gain) = calc_l_r_gain(1.0f, -0.5f);
    EXPECT_NEAR(1.2f, left_gain, 0.01f);
    EXPECT_FLOAT_EQ(0.5, right_gain);
}
