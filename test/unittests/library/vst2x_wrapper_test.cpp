#include "gtest/gtest.h"

#include "test_utils.h"

#include "library/vst2x_wrapper.cpp"

using namespace sushi;
using namespace sushi::vst2;

class TestVst2xWrapper : public ::testing::Test
{
protected:
    TestVst2xWrapper()
    {
    }

    void SetUp()
    {
        char* full_again_path = realpath("libagain.so", NULL);
        _module_under_test = new Vst2xWrapper(full_again_path);
        free(full_again_path);

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

TEST_F(TestVst2xWrapper, TestSetName)
{
    EXPECT_EQ("Gain", _module_under_test->name());
    EXPECT_EQ("Gain", _module_under_test->label());
}

TEST_F(TestVst2xWrapper, TestSetChannels)
{
    EXPECT_EQ(2, _module_under_test->input_channels());
    EXPECT_EQ(2, _module_under_test->output_channels());
}

TEST_F(TestVst2xWrapper, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0, out_buffer);
}
