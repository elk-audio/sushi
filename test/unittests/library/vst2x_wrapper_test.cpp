#include "gtest/gtest.h"

#define private public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#include "test_utils/engine_mockup.h"
#include "library/vst2x_wrapper.cpp"

using namespace sushi;
using namespace sushi::vst2;

// Reference output signal from TestPlugin
// in response to NoteON C4 (60), vel=127, default parameters
static constexpr float TEST_SYNTH_EXPECTED_OUT[2][64] = {
        {
                1, 0.999853, 0.999414, 0.998681, 0.997655, 0.996337, 0.994727, 0.992825,
                0.990632, 0.988149, 0.985375, 0.982313, 0.978963, 0.975326, 0.971403,
                0.967195, 0.962703, 0.95793, 0.952875, 0.947541, 0.941929, 0.936041,
                0.929879, 0.923443, 0.916738, 0.909763, 0.902521, 0.895015, 0.887247,
                0.879218, 0.870932, 0.86239, 0.853596, 0.844551, 0.835258, 0.825721,
                0.815941, 0.805923, 0.795668, 0.785179, 0.774461, 0.763515, 0.752346,
                0.740956, 0.729348, 0.717527, 0.705496, 0.693257, 0.680815, 0.668174,
                0.655337, 0.642307, 0.62909, 0.615688, 0.602105, 0.588346, 0.574414,
                0.560314, 0.546049, 0.531625, 0.517045, 0.502313, 0.487433, 0.472411
        },
        {
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
                0.5f, 0.5f, 0.5f, 0.5f
        }
};

constexpr float TEST_SAMPLE_RATE = 48000;

class TestVst2xWrapper : public ::testing::Test
{
protected:
    TestVst2xWrapper()
    {
    }

    void SetUp(const std::string& plugin_path)
    {
        char* full_plugin_path = realpath(plugin_path.c_str(), NULL);
        _module_under_test = new Vst2xWrapper(_host_control.make_host_control_mockup(TEST_SAMPLE_RATE), full_plugin_path);
        free(full_plugin_path);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
    Vst2xWrapper* _module_under_test;
};


TEST_F(TestVst2xWrapper, TestSetName)
{
    SetUp("libvst2_test_plugin.so");
    EXPECT_EQ("Test Plugin", _module_under_test->name());
    EXPECT_EQ("Test Plugin", _module_under_test->label());
}

TEST_F(TestVst2xWrapper, TestSetChannels)
{
    SetUp("libvst2_test_plugin.so");
    EXPECT_EQ(2, _module_under_test->input_channels());
    EXPECT_EQ(2, _module_under_test->output_channels());

    _module_under_test->set_input_channels(1);
    _module_under_test->set_output_channels(1);

    EXPECT_EQ(1, _module_under_test->input_channels());
    EXPECT_EQ(1, _module_under_test->output_channels());
}

TEST_F(TestVst2xWrapper, TestParameterInitialization)
{
    SetUp("libvst2_test_plugin.so");
    auto gain_param = _module_under_test->parameter_from_name("Gain");
    EXPECT_TRUE(gain_param);
    EXPECT_EQ(0u, gain_param->id());
    EXPECT_EQ("Gain", gain_param->name());
    EXPECT_EQ("Gain", gain_param->label());
    EXPECT_EQ("dB", gain_param->unit());
}

TEST_F(TestVst2xWrapper, TestPluginCanDos)
{
    SetUp("libvst2_test_plugin.so");
    EXPECT_FALSE(_module_under_test->_can_do_soft_bypass);
}

TEST_F(TestVst2xWrapper, TestParameterSetViaEvent)
{
    SetUp("libvst2_test_plugin.so");
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.123f);
    _module_under_test->process_event(event);
    auto handle = _module_under_test->_plugin_handle;
    EXPECT_EQ(0.123f, handle->getParameter(handle, 0));
}

TEST_F(TestVst2xWrapper, TestProcess)
{
    SetUp("libvst2_test_plugin.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestVst2xWrapper, TestMonoProcess)
{
    SetUp("libvst2_test_plugin.so");
    ChunkSampleBuffer mono_buffer(1);
    ChunkSampleBuffer stereo_buffer(2);

    _module_under_test->set_input_channels(1);
    EXPECT_TRUE(_module_under_test->_double_mono_input);
    test_utils::fill_sample_buffer(mono_buffer, 1.0f);
    _module_under_test->process_audio(mono_buffer, stereo_buffer);
    test_utils::assert_buffer_value(1.0f, stereo_buffer);

    _module_under_test->set_output_channels(1);
    _module_under_test->set_input_channels(2);
    test_utils::fill_sample_buffer(stereo_buffer, 2.0f);
    _module_under_test->process_audio(stereo_buffer, mono_buffer);
    test_utils::assert_buffer_value(2.0f, mono_buffer);
}

TEST_F(TestVst2xWrapper, TestProcessingWithParameterChanges)
{
    SetUp("libvst2_test_plugin.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.123f);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Verify that a parameter change affects the sound
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.123f, out_buffer);

    // Verify that we can retrive the new value
    auto [status, value] = _module_under_test->parameter_value(0);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    ASSERT_FLOAT_EQ(0.123f, value);
}

TEST_F(TestVst2xWrapper, TestBypassProcessing)
{
    SetUp("libvst2_test_plugin.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    // Set the gain to 0.5
    auto event = RtEvent::make_parameter_change_event(0, 0, 0, 0.5f);
    _module_under_test->process_event(event);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    // Set bypass and manually feed the generated RtEvent back to the
    // wrapper processor as event dispatcher is not running
    _module_under_test->set_bypassed(true);
    auto bypass_event = _host_control._dummy_dispatcher.retrieve_event();
    EXPECT_TRUE(bypass_event.get());
    _module_under_test->process_event(bypass_event->to_rt_event(0));
    EXPECT_TRUE(_module_under_test->bypassed());
    _module_under_test->process_audio(in_buffer, out_buffer);

    // Test that we are ramping up the audio to the bypass value
    float prev_value = 0;
    for (int i = 1; i< AUDIO_CHUNK_SIZE; ++i)
    {
        EXPECT_GT(out_buffer.channel(0)[i], prev_value);
        prev_value = out_buffer.channel(0)[i];
    }
}

TEST_F(TestVst2xWrapper, TestTimeInfo)
{
    SetUp("libvst2_test_plugin.so");
    _host_control._transport.set_tempo(60);
    _host_control._transport.set_time_signature({4, 4});
    _host_control._transport.set_time(std::chrono::seconds(1), static_cast<int64_t>(TEST_SAMPLE_RATE));
    auto time_info = _module_under_test->time_info();
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE), time_info->samplePos);
    EXPECT_EQ(1'000'000'000, time_info->nanoSeconds);
    EXPECT_FLOAT_EQ(1.0f, time_info->ppqPos);
    EXPECT_FLOAT_EQ(60.0f, time_info->tempo);
    EXPECT_FLOAT_EQ(0.0f, time_info->barStartPos);
    EXPECT_EQ(4, time_info->timeSigNumerator);
    EXPECT_EQ(4, time_info->timeSigDenominator);
}

TEST_F(TestVst2xWrapper, TestMidiEvents)
{
    SetUp("libvst2_test_plugin.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    _module_under_test->process_event(RtEvent::make_note_on_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
    for (int i=0; i<2; i++)
    {
        for (int j=0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
            ASSERT_NEAR(TEST_SYNTH_EXPECTED_OUT[i][j], out_buffer.channel(i)[j], 0.00001);
        }
    }

    // Send NoteOFF, plugin should immediately silence everything
    _module_under_test->process_event(RtEvent::make_note_off_event(0, 0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestVst2xWrapper, TestConfigurationChange)
{
    SetUp("libvst2_test_plugin.so");
    _module_under_test->configure(44100.0f);
    ASSERT_FLOAT_EQ(44100, _module_under_test->_sample_rate);
}

TEST_F(TestVst2xWrapper, TestParameterChangeNotifications)
{
    SetUp("libvst2_test_plugin.so");
    ASSERT_FALSE(_host_control._dummy_dispatcher.got_event());
    _module_under_test->notify_parameter_change(0, 0.5f);
    auto event = std::move(_host_control._dummy_dispatcher.retrieve_event());
    ASSERT_FALSE(event == nullptr);
    ASSERT_TRUE(event->is_parameter_change_notification());
}

TEST_F(TestVst2xWrapper, TestRTParameterChangeNotifications)
{
    SetUp("libvst2_test_plugin.so");
    RtSafeRtEventFifo queue;
    _module_under_test->set_event_output(&queue);
    ASSERT_TRUE(queue.empty());
    _module_under_test->notify_parameter_change_rt(0, 0.5f);
    RtEvent event;
    auto received = queue.pop(event);
    ASSERT_TRUE(received);
    ASSERT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
}

TEST_F(TestVst2xWrapper, TestProgramManagement)
{
    SetUp("libvst2_test_plugin.so");
    ASSERT_TRUE(_module_under_test->supports_programs());
    ASSERT_EQ(3, _module_under_test->program_count());
    ASSERT_EQ(0, _module_under_test->current_program());
    ASSERT_EQ("Program 1", _module_under_test->current_program_name());
    auto [status, program_name] = _module_under_test->program_name(2);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    ASSERT_EQ("Program 3", program_name);
    // Access with an invalid program number
    std::tie(status, program_name) = _module_under_test->program_name(2000);
    ASSERT_NE(ProcessorReturnCode::OK, status);
    // Get all programs,
    auto [res, programs] = _module_under_test->all_program_names();
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    ASSERT_EQ("Program 2", programs[1]);
    ASSERT_EQ(3u, programs.size());
}
