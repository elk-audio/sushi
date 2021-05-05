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
    ASSERT_GT(send_plugin.use_count(), 0);
    ASSERT_GT(send_plugin->id(), 0u);

    info.uid = "sushi.testing.return";
    auto [ret_status, return_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, ret_status);
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
    TestSendReturnPlugins()
    {
    }
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

TEST_F(TestSendReturnPlugins, TestProcessing)
{
    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_1, 1.0f);

    // Test that processing without destination doesn't break and passes though
    _send_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(1.0f, buffer_2);

    _send_instance.set_destination(&_return_instance);
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

    _send_instance.set_destination(&_return_instance);
    _send_instance.process_audio(buffer_1, buffer_2);

    SendPlugin send_instance_2(_host_ctrl, &_factory);
    send_instance_2.set_destination(&_return_instance);
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

TEST_F(TestSendReturnPlugins, TestMonoToStereoSend)
{
    ChunkSampleBuffer buffer_mono(1);
    ChunkSampleBuffer buffer_1(2);
    ChunkSampleBuffer buffer_2(2);
    test_utils::fill_sample_buffer(buffer_mono, 1.0f);

    _send_instance.set_input_channels(1);
    _send_instance.set_output_channels(1);
    _send_instance.set_destination(&_return_instance);
    _send_instance.process_audio(buffer_mono, buffer_mono);

    // Swap manually and verify that signal is returned
    _return_instance._swap_buffers();
    _return_instance.process_audio(buffer_1, buffer_2);
    test_utils::assert_buffer_value(1.0f, buffer_2);
}