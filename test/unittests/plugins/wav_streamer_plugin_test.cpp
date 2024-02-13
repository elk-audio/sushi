#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/rt_event_fifo.h"

#include "sushi/warning_suppressor.h"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
ELK_POP_WARNING

#include "plugins/wav_streamer_plugin.cpp"

using namespace sushi;
using namespace sushi::internal::wav_streamer_plugin;

constexpr float TEST_SAMPLERATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;

static const std::string SAMPLE_FILE = "Kawai-K11-GrPiano-C4_mono.wav";
constexpr float SAMPLE_FILE_LENGTH = 1.779546485f;

TEST(TestWaveStreamerPluginInternal, TestGainScaleFunction)
{
    EXPECT_NEAR(0.125f, exp_approx(0.5f, 1.0f), 0.001f);
    EXPECT_NEAR(0.015625, exp_approx(0.25f, 1.0f), 0.001f);
    EXPECT_NEAR(0.25f, exp_approx(1.0f, 2.0f), 0.001f);
    EXPECT_NEAR(0.0625f, exp_approx(0.25f, 0.5f), 0.001f);
}

class TestWaveStreamerPlugin : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _module_under_test = std::make_unique<WavStreamerPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_input_channels(0);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_enabled(true);
        _module_under_test->set_event_output(&_fifo);
    }

    void LoadFile(const std::string& file)
    {
        auto path = std::string(test_utils::get_data_dir_path().append(file));

        auto res = _module_under_test->set_property_value(FILE_PROPERTY_ID, path);
        ASSERT_EQ(ProcessorReturnCode::OK, res);
    }

    HostControlMockup _host_control;
    std::unique_ptr<WavStreamerPlugin> _module_under_test;
    RtSafeRtEventFifo _fifo;
};

TEST_F(TestWaveStreamerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Wav Streamer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.wav_streamer", _module_under_test->name());

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0, out_buffer);
}

TEST_F(TestWaveStreamerPlugin, TestWaveLoadingAndPlaying)
{
    auto playing_param_id = _module_under_test->parameter_from_name("playing")->id();
    auto len_param_id = _module_under_test->parameter_from_name("length")->id();
    auto pos_param_id = _module_under_test->parameter_from_name("position")->id();

    // Load non-existing file
    LoadFile("NO FILE");
    EXPECT_EQ("Error:", _module_under_test->property_value(FILE_PROPERTY_ID).second.substr(0, 6));
    EXPECT_EQ(0.0f, _module_under_test->parameter_value(len_param_id).second);
    EXPECT_EQ(0.0f, _module_under_test->parameter_value(pos_param_id).second);

    // Load actual file and verify we got something on the output
    LoadFile(SAMPLE_FILE);
    _module_under_test->process_event(RtEvent::make_parameter_change_event(0, 0, playing_param_id, 1.0f));

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    for (int i = 0; i <= SEEK_UPDATE_INTERVAL; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
        test_utils::assert_buffer_non_null(out_buffer);
    }

    // Verify that output parameters are updated
    EXPECT_FLOAT_EQ(SAMPLE_FILE_LENGTH / MAX_FILE_LENGTH, _module_under_test->parameter_value(len_param_id).second);
    EXPECT_EQ("1.78", _module_under_test->parameter_value_formatted(len_param_id).second);
    EXPECT_NE(0.0f, _module_under_test->parameter_value(pos_param_id).second);
    EXPECT_EQ(1.0f, _module_under_test->parameter_value(playing_param_id).second);

    // Load non-existing file again and verify that playback stops and parameters are updated.
    LoadFile("NO FILE");
    for (int i = 0; i <= + SEEK_UPDATE_INTERVAL * 2 ; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }
    EXPECT_EQ(0.0f, _module_under_test->parameter_value(len_param_id).second);
    EXPECT_EQ(0.0f, _module_under_test->parameter_value(pos_param_id).second);
    EXPECT_EQ(0.0f, _module_under_test->parameter_value(playing_param_id).second);
}


