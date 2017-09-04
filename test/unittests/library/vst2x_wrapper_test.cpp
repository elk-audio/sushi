#include "gtest/gtest.h"

#define private public

#include "test_utils.h"

#include "library/vst2x_wrapper.cpp"

using namespace sushi;
using namespace sushi::vst2;

// Reference output signal from VstXSynth
// in response to NoteON C4 (60), vel=127, default parameters
static constexpr float VSTXSYNTH_EXPECTED_OUT[2][64] = {
        {
                -0.29699999f, -0.29380956f, -0.29061913f, -0.28742871f, -0.28409326f, -0.28090283f,
                -0.27771240f, -0.27437696f, -0.27118653f, -0.26799610f, -0.26466063f, -0.26147023f,
                -0.25827980f, -0.25494432f, -0.25175390f, -0.24856347f, -0.24522804f, -0.24203759f,
                -0.23884717f, -0.23551174f, -0.23232129f, -0.22913086f, -0.22579540f, -0.22260499f,
                -0.21941455f, -0.21607910f, -0.21288869f, -0.20969824f, -0.20636280f, -0.20317237f,
                -0.19998193f, -0.19664648f, -0.19345607f, -0.19026563f, -0.18693018f, -0.18373975f,
                -0.18054931f, -0.17721386f, -0.17402345f, -0.17083301f, -0.16749756f, -0.16430713f,
                -0.16111670f, -0.15792628f, -0.15459082f, -0.15140040f, -0.14820996f, -0.14487451f,
                -0.14168409f, -0.13849366f, -0.13515820f, -0.13196778f, -0.12877734f, -0.12544189f,
                -0.12225147f, -0.11906105f, -0.11572558f, -0.11253516f, -0.10934473f, -0.10600928f,
                -0.10281885f, -0.09962842f, -0.09629297f, -0.09310254f
        },
        {
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f,
                -0.29699999f, -0.29699999f, -0.29699999f, -0.29699999f, 0.29699999f, 0.29699999f,
                0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f,
                0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f,
                0.29699999f, 0.29699999f, 0.29699999f, 0.29699999f
        }
};

class TestVst2xWrapper : public ::testing::Test
{
protected:
    TestVst2xWrapper()
    {
    }

    void SetUp(const std::string& plugin_path)
    {
        char* full_plugin_path = realpath(plugin_path.c_str(), NULL);
        _module_under_test = new Vst2xWrapper(full_plugin_path);
        free(full_plugin_path);

        auto ret = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    Vst2xWrapper* _module_under_test;
};

// TODO:
//      Steinberg's again SDK plugin used here is very limited in functionalities,
//      so we can't test all host controls. Add more tests after preparing an ad-hoc
//      test plugin.

TEST_F(TestVst2xWrapper, test_set_name)
{
    SetUp("libagain.so");
    EXPECT_EQ("Gain", _module_under_test->name());
    EXPECT_EQ("Gain", _module_under_test->label());
}

TEST_F(TestVst2xWrapper, test_set_channels)
{
    SetUp("libagain.so");
    EXPECT_EQ(2, _module_under_test->input_channels());
    EXPECT_EQ(2, _module_under_test->output_channels());

    EXPECT_TRUE(_module_under_test->set_input_channels(1));
    EXPECT_TRUE(_module_under_test->set_output_channels(1));

    EXPECT_EQ(1, _module_under_test->input_channels());
    EXPECT_EQ(1, _module_under_test->output_channels());
}

TEST_F(TestVst2xWrapper, test_parameter_initialization)
{
    SetUp("libagain.so");
    auto gain_param = _module_under_test->parameter_from_name("Gain");
    EXPECT_TRUE(gain_param);
    EXPECT_EQ(0u, gain_param->id());
}

TEST_F(TestVst2xWrapper, test_plugin_can_dos)
{
    SetUp("libagain.so");
    EXPECT_FALSE(_module_under_test->_can_do_soft_bypass);
}

TEST_F(TestVst2xWrapper, test_parameter_set_via_event)
{
    SetUp("libagain.so");
    auto event = Event::make_parameter_change_event(0, 0, 0, 0.123f);
    _module_under_test->process_event(event);
    auto handle = _module_under_test->_plugin_handle;
    EXPECT_EQ(0.123f, handle->getParameter(handle, 0));
}

TEST_F(TestVst2xWrapper, test_process)
{
    SetUp("libagain.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestVst2xWrapper, test_mono_process)
{
    SetUp("libagain.so");
    ChunkSampleBuffer mono_buffer(1);
    ChunkSampleBuffer stereo_buffer(2);

    ASSERT_TRUE(_module_under_test->set_input_channels(1));
    EXPECT_TRUE(_module_under_test->_double_mono_input);
    test_utils::fill_sample_buffer(mono_buffer, 1.0f);
    _module_under_test->process_audio(mono_buffer, stereo_buffer);
    test_utils::assert_buffer_value(1.0f, stereo_buffer);

    ASSERT_TRUE(_module_under_test->set_output_channels(1));
    ASSERT_TRUE(_module_under_test->set_input_channels(2));
    EXPECT_TRUE(_module_under_test->_sum_stereo_to_mono_output);
    test_utils::fill_sample_buffer(stereo_buffer, 2.0f);
    _module_under_test->process_audio(stereo_buffer, mono_buffer);
    test_utils::assert_buffer_value(2.0f, mono_buffer);
}

TEST_F(TestVst2xWrapper, test_processing_with_parameter_changes)
{
    SetUp("libagain.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    auto event = Event::make_parameter_change_event(0, 0, 0, 0.123f);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.123f, out_buffer);
}

TEST_F(TestVst2xWrapper, test_bypass_processing)
{
    SetUp("libagain.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    auto event = Event::make_parameter_change_event(0, 0, 0, 5.0f);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _module_under_test->set_bypassed(true);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestVst2xWrapper, test_midi_events)
{
    SetUp("libvstxsynth.so");
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);

    _module_under_test->process_event(Event::make_note_on_event(0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
    for (int i=0; i<2; i++)
    {
        for (int j=0; j < std::min(AUDIO_CHUNK_SIZE, 64); j++)
        {
            ASSERT_FLOAT_EQ(VSTXSYNTH_EXPECTED_OUT[i][j], out_buffer.channel(i)[j]);
        }
    }

    // Send NoteOFF, VstXsynth immediately silence everything
    _module_under_test->process_event(Event::make_note_off_event(0, 0, 60, 1.0f));
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestVst2xWrapper, test_configuration_change)
{
    SetUp("libagain.so");
    _module_under_test->configure(44100.0f);
    ASSERT_FLOAT_EQ(44100, _module_under_test->_sample_rate);
}
