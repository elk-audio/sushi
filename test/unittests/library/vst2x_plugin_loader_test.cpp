#include "gtest/gtest.h"

#include "test_utils/host_control_mockup.h"
#include "library/plugin_registry.cpp"
#include "library/vst2x/vst2x_processor_factory.cpp"
#include "library/vst3x/vst3x_processor_factory.cpp"
#include "library/lv2/lv2_processor_factory.cpp"
#include "library/internal_processor_factory.cpp"

#include "library/vst2x/vst2x_plugin_loader.cpp"

constexpr float SAMPLE_RATE = 44000;

using namespace sushi;

// Empty fixture as PluginLoader has only static methods so far

class TestVst2xPluginLoader : public ::testing::Test
{
protected:
    TestVst2xPluginLoader():
            _host_control(_hc.make_host_control_mockup(SAMPLE_RATE))
    {

    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    sushi::HostControl _host_control;
    HostControlMockup _hc;
    PluginRegistry _plugin_registry;
};

// TODO Ilias: Should this still be called plugin_loader_test when AUD-294 is done?

TEST_F(TestVst2xPluginLoader, TestPluginRegistryVst2xLoading)
{
    char* full_again_path = realpath("libvst2_test_plugin.so", NULL);

    PluginInfo plugin_info;
    plugin_info.uid = "";
    // dlopen on Linux requires absolute paths if library is not on system paths already
    plugin_info.path = full_again_path;
    plugin_info.type = PluginType::VST2X;

    auto [processor_status, processor] = _plugin_registry.new_instance(plugin_info,
                                                                       _host_control,
                                                                       SAMPLE_RATE);

    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);

    free(full_again_path);
}


TEST_F(TestVst2xPluginLoader, TestLoadPlugin)
{
    // dlopen on Linux requires absolute paths if library is not on system paths already
    char* full_again_path = realpath("libvst2_test_plugin.so", NULL);
    auto library_handle = vst2::PluginLoader::get_library_handle_for_plugin(full_again_path);
    ASSERT_NE(nullptr, library_handle);
    free(full_again_path);
    auto plugin = vst2::PluginLoader::load_plugin(library_handle);

    // Check magic number
    EXPECT_EQ(kEffectMagic, plugin->magic);

    vst2::PluginLoader::close_library_handle(library_handle);
}

