#include "gtest/gtest.h"

#define private public

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

TEST_F(TestVst2xWrapper, TestSetName)
{
    SetUp("libagain.so");
    EXPECT_EQ("Gain", _module_under_test->name());
    EXPECT_EQ("Gain", _module_under_test->label());
}


TEST_F(TestVst2xWrapper, TestSetChannels)
{
    SetUp("libagain.so");
    EXPECT_EQ(2, _module_under_test->input_channels());
    EXPECT_EQ(2, _module_under_test->output_channels());
}


TEST_F(TestVst2xWrapper, TestParameterInitialization)
{
    SetUp("libagain.so");
    EXPECT_EQ(1u, _module_under_test->_param_names_to_id.size());
    ProcessorReturnCode status;
    ObjectId id;
    std::tie(status, id) = _module_under_test->parameter_id_from_name("Gain");
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(0u, id);
}


TEST_F(TestVst2xWrapper, TestParameterSetViaEvent)
{
    SetUp("libagain.so");
    auto event = Event::make_parameter_change_event(0, 0, 0, 0.123f);
    _module_under_test->process_event(event);
    auto handle = _module_under_test->_plugin_handle;
    EXPECT_EQ(0.123f, handle->getParameter(handle, 0));
}


TEST_F(TestVst2xWrapper, TestProcess)
{
    SetUp("libagain.so");
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);
}


TEST_F(TestVst2xWrapper, TestProcessingWithParameterChanges)
{
    SetUp("libagain.so");
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    auto event = Event::make_parameter_change_event(0, 0, 0, 0.123f);

    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.123f, out_buffer);
}

