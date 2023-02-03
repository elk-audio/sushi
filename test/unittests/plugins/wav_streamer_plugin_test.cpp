#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/rt_event_fifo.h"

#define private public

#include "plugins/wav_streamer_plugin.cpp"

using namespace sushi;
using namespace sushi::wav_streamer_plugin;

constexpr float TEST_SAMPLERATE = 44100;
constexpr int   TEST_CHANNEL_COUNT = 2;

static const std::string SAMPLE_FILE = "Kawai-K11-GrPiano-C4_mono.wav";


/* Test the Voice class */
class TestWaveStreamerPlugin : public ::testing::Test
{
protected:
    void SetUp()
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

TEST_F(TestWaveStreamerPlugin, TestWaveLoading)
{
    LoadFile(SAMPLE_FILE);

    auto param_id = _module_under_test->parameter_from_name("playing")->id();
    _module_under_test->process_event(RtEvent::make_parameter_change_event(0, 0, param_id, 1.0f));

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_non_null(out_buffer);
}


