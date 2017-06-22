#include "gtest/gtest.h"

#include "library/vst3x_host_app.cpp"
#include "library/vst3x_wrapper.cpp"
#include "library/plugin_events.h"
#include "library/sample_buffer.h"
#include "test_utils.h"

using namespace sushi;
using namespace sushi::vst3;

char PLUGIN_FILE[] = "../VST3/adelay.vst3";


/* Quick test to test plugin loading */
TEST(TestVst3xPluginLoader, test_load_plugin)
{
    char* full_test_plugin_path = realpath(PLUGIN_FILE, NULL);
    PluginLoader module_under_test(full_test_plugin_path);
    bool success;
    PluginInstance instance;
    std::tie(success, instance) = module_under_test.load_plugin();
    ASSERT_TRUE(success);
    ASSERT_TRUE(instance.processor());
    ASSERT_TRUE(instance.component());
    ASSERT_TRUE(instance.controller());

    free(full_test_plugin_path);
}

/* Test that nothing breaks if the plugin is not found */
TEST(TestVst3xPluginLoader, test_load_plugin_from_erroneous_filename)
{
    PluginLoader module_under_test("/usr/lib/lxvst/no_plugin.vst3");
    bool success;
    PluginInstance instance;
    std::tie(success, instance) = module_under_test.load_plugin();
    ASSERT_FALSE(success);
}

class TestVst3xWrapper : public ::testing::Test
{
protected:
    TestVst3xWrapper()
    {
    }

    void SetUp()
    {
        char* full_plugin_path = realpath(PLUGIN_FILE, NULL);
        _module_under_test = new Vst3xWrapper(full_plugin_path);
        free(full_plugin_path);

        auto ret = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    Vst3xWrapper* _module_under_test;
};

TEST_F(TestVst3xWrapper, test_load_and_init_plugin)
{
    ASSERT_TRUE(_module_under_test);
    EXPECT_EQ("ADelay", _module_under_test->name());

    auto parameters = _module_under_test->all_parameters();
    EXPECT_EQ(2u, parameters.size());
    EXPECT_EQ("Bypass", parameters[0]->name());
    EXPECT_EQ("Delay", parameters[1]->name());
}

TEST_F(TestVst3xWrapper, test_processing)
{
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 0);
    auto event = Event::make_parameter_change_event(0u, 0, 1, 0.0f);

    _module_under_test->set_enabled(true);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(0, out_buffer);
}
