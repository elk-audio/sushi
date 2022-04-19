#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#define private public

#include "plugins/send_return_factory.cpp"
#include "plugins/send_plugin.cpp"
#include "plugins/return_plugin.cpp"

using namespace sushi;
using namespace sushi::send_plugin;
using namespace sushi::return_plugin;

constexpr float TEST_SAMPLERATE = 44100;

TEST(TestSendReturnFactory, TestFactoryCreation)
{
    SendReturnFactory factory;
    HostControlMockup host_control_mockup;
    auto host_ctrl = host_control_mockup.make_host_control_mockup(TEST_SAMPLERATE);
    PluginInfo info{.uid = "sushi.testing.send", .path = "", .type = PluginType::INTERNAL};

    auto [send_status, send_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, send_status);
    ASSERT_EQ("Send", send_plugin->label());
    ASSERT_GT(send_plugin.use_count(), 0);
    ASSERT_GT(send_plugin->id(), 0u);

    info.uid = "sushi.testing.return";
    auto [ret_status, return_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, ret_status);
    ASSERT_EQ("Return", return_plugin->label());
    ASSERT_GT(return_plugin.use_count(), 0);
    ASSERT_GT(return_plugin->id(), 0u);

    // Negative test
    info.uid = "sushi.testing.aux_";
    auto [error_status, error_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_NE(ProcessorReturnCode::OK, error_status);
    ASSERT_FALSE(error_plugin);
}


class TestSendReturnPlugins : public ::testing::Test
{
protected:
    TestSendReturnPlugins() = default;

    void SetUp()
    {
        ASSERT_EQ(ProcessorReturnCode::OK, _send_instance.init(TEST_SAMPLERATE));
        _send_instance.set_input_channels(2);
        _send_instance.set_output_channels(2);

        ASSERT_EQ(ProcessorReturnCode::OK, _return_instance.init(TEST_SAMPLERATE));
        _return_instance.set_input_channels(2);
        _return_instance.set_output_channels(2);
    }

    void TearDown()
    {
    }

    SendReturnFactory   _factory;
    HostControlMockup   _host_control_mockup;
    HostControl         _host_ctrl{_host_control_mockup.make_host_control_mockup(TEST_SAMPLERATE)};

    SendPlugin          _send_instance{_host_ctrl, &_factory};
    ReturnPlugin        _return_instance{_host_ctrl, &_factory};
};

TEST_F(TestSendReturnPlugins, TestDestinationSetting)
{
    PluginInfo info;
    info.uid = "sushi.testing.return";
    auto [status, return_instance_2] = _factory.new_instance(info, _host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    _return_instance.set_name("return_1");
    return_instance_2->set_name("return_2");

    EXPECT_EQ(DEFAULT_DEST, _send_instance.property_value(DEST_PROPERTY_ID).second);
    status = _send_instance.set_property_value(DEST_PROPERTY_ID, "return_2");
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(_send_instance._destination, return_instance_2.get());
    EXPECT_EQ("return_2", _send_instance.property_value(DEST_PROPERTY_ID).second);

    // Destroy the second return and it should be automatically unlinked.
    return_instance_2.reset();
    EXPECT_EQ(_send_instance._destination, nullptr);
    EXPECT_EQ(DEFAULT_DEST, _send_instance.property_value(DEST_PROPERTY_ID).second);
}

TEST_F(TestSendReturnPlugins, TestProcessing)
{
    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);

    // Test that processing without destination doesn't break and passes though
    _send_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(1.0f, buffer_2);

    _send_instance._set_destination(&_return_instance);
    _send_instance.process_audio(buffer_1, buffer_2);
    buffer_2.clear();

    // Swap manually and verify that signal is returned
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(1.0f, buffer_2);
}

TEST_F(TestSendReturnPlugins, TestMultipleSends)
{
    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);

    _host_control_mockup._transport.set_time(Time(0), 0);

    _send_instance._set_destination(&_return_instance);
    _send_instance.process_audio(buffer_1, buffer_2);

    SendPlugin send_instance_2(_host_ctrl, &_factory);
    send_instance_2._set_destination(&_return_instance);
    send_instance_2.process_audio(buffer_1, buffer_2);
    buffer_2.clear();

    // Call process on the return, the buffers should not be swapped so output should be 0
    _return_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(0.0f, buffer_2);

    // Fast forward time and call process again, buffers should now be swapped and we should
    // read both sends on the output
    _host_control_mockup._transport.set_time(Time(10), AUDIO_CHUNK_SIZE);
    _return_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(2.0f, buffer_2);
}

TEST_F(TestSendReturnPlugins, TestSelectiveChannelSending)
{
    auto channel_count_param_id = _send_instance.parameter_from_name("channel_count")->id();
    auto start_channel_param_id = _send_instance.parameter_from_name("start_channel")->id();
    auto dest_channel_param_id = _send_instance.parameter_from_name("dest_channel")->id();


    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);

    _send_instance.set_input_channels(2);
    _send_instance.set_output_channels(2);
    _send_instance._set_destination(&_return_instance);

    // Send only 1 channel
    auto event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, channel_count_param_id,
                                                      1.0 / (MAX_TRACK_CHANNELS - 1));
    _send_instance.process_event(event);
    _send_instance.process_audio(buffer_1, buffer_1);

    // Swap manually and verify that signal only the first channel was sent
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    EXPECT_FLOAT_EQ(1.0f, buffer_2.channel(0)[0]);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(1)[0]);

    // Set the destination channel to channel 1
    event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, dest_channel_param_id,
                                                 1.0 / (MAX_TRACK_CHANNELS - 1));
    _send_instance.process_event(event);
    _send_instance.process_audio(buffer_1, buffer_1);

    // Swap manually and verify that signal only the first channel was sent to channel 2
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(0)[0]);
    EXPECT_FLOAT_EQ(1.0f, buffer_2.channel(1)[0]);

    // Set a destination channel outside the range of the return plugin's channel range
    event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, dest_channel_param_id, 1.0);
    _send_instance.process_event(event);
    _send_instance.process_audio(buffer_1, buffer_1);

    // Both return channels should be 0
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(0)[0]);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(1)[0]);

    // Send both channels the send plugin to channels 3 & 4 of the return plugin
    _return_instance.set_input_channels(4);
    _return_instance.set_output_channels(4);

    buffer_1.channel(0)[0] = 2.0;
    buffer_1.channel(1)[0] = 3.0;
    event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, start_channel_param_id, 0);
    _send_instance.process_event(event);
    event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, dest_channel_param_id,
                                                 2.0 / (MAX_TRACK_CHANNELS - 1));
    _send_instance.process_event(event);
    event = RtEvent::make_parameter_change_event(_send_instance.id(), 0, channel_count_param_id,
                                                 2.0 / (MAX_TRACK_CHANNELS - 1));
    _send_instance.process_event(event);

    _send_instance.process_audio(buffer_1, buffer_1);

    buffer_1 = ChunkSampleBuffer(4);
    buffer_2 = ChunkSampleBuffer(4);

    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(0)[0]);
    EXPECT_FLOAT_EQ(0.0f, buffer_2.channel(1)[0]);
    EXPECT_FLOAT_EQ(2.0f, buffer_2.channel(2)[0]);
    EXPECT_FLOAT_EQ(3.0f, buffer_2.channel(3)[0]);
}

TEST_F(TestSendReturnPlugins, TestRampedProcessing)
{
    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);

    // Test only ramping
    _return_instance.send_audio_with_ramp(buffer_1, 0, 2.0f, 0.0f);
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    EXPECT_NEAR(2.0f, buffer_2.channel(0)[0], 0.01);
    EXPECT_NEAR(1.0f, buffer_2.channel(0)[AUDIO_CHUNK_SIZE / 2], 0.1);
    EXPECT_NEAR(0.0f, buffer_2.channel(0)[AUDIO_CHUNK_SIZE - 1], 0.01);
    _return_instance._swap_buffers();

    // Test parameter smoothing
    _send_instance._set_destination(&_return_instance);
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.0f);
    _send_instance.process_event(event);
    _send_instance.process_audio(buffer_1, buffer_2);
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);

    // Audio should now begin to ramp down
    EXPECT_FLOAT_EQ(1.0f, buffer_2.channel(0)[0]);
    EXPECT_LT(buffer_2.channel(0)[AUDIO_CHUNK_SIZE -1], 1.0f);
    EXPECT_GT(buffer_2.channel(0)[AUDIO_CHUNK_SIZE / 2], buffer_2.channel(0)[AUDIO_CHUNK_SIZE - 1]);
 }