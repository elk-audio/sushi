#include <filesystem>

#include "gtest/gtest.h"

#include "test_utils/host_control_mockup.h"
#include "library/plugin_registry.h"

#include "library/vst2x/vst2x_plugin_loader.cpp"

constexpr float SAMPLE_RATE = 44000;

using namespace sushi;

// Empty fixture as PluginLoader has only static methods so far

class TestVst2xPluginLoading : public ::testing::Test
{
protected:
    TestVst2xPluginLoading():
            _host_control(_hc.make_host_control_mockup(SAMPLE_RATE))
    {

    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    HostControlMockup _hc;
    sushi::HostControl _host_control;
    PluginRegistry _plugin_registry;
};

TEST_F(TestVst2xPluginLoading, TestPluginRegistryVst2xLoading)
{
    auto full_path = std::filesystem::path(VST2_TEST_PLUGIN_PATH);
    auto full_again_path = std::string(std::filesystem::absolute(full_path));

    PluginInfo plugin_info;
    plugin_info.uid = "";
    // dlopen on Linux requires absolute paths if library is not on system paths already
    plugin_info.path = full_again_path;
    plugin_info.type = PluginType::VST2X;

    auto [processor_status, processor] = _plugin_registry.new_instance(plugin_info,
                                                                       _host_control,
                                                                       SAMPLE_RATE);

    ASSERT_EQ(processor_status, ProcessorReturnCode::OK);
}

TEST_F(TestVst2xPluginLoading, TestLoadPlugin)
{
    // dlopen on Linux requires absolute paths if library is not on system paths already
    auto full_path = std::filesystem::path(VST2_TEST_PLUGIN_PATH);
    auto full_again_path = std::string(std::filesystem::absolute(full_path));

    auto library_handle = vst2::PluginLoader::get_library_handle_for_plugin(full_again_path);
    ASSERT_NE(nullptr, library_handle);
    auto plugin = vst2::PluginLoader::load_plugin(library_handle);

    // Check magic number
    EXPECT_EQ(kEffectMagic, plugin->magic);

    vst2::PluginLoader::close_library_handle(library_handle);
}
