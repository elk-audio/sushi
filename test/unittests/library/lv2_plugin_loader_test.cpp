#include "gtest/gtest.h"

#include "library/lv2_plugin_loader.cpp"

using namespace sushi;
using namespace sushi::lv2;

// Ilias TODO: Check this also for LV2, maybe it is no longer the case.
// Empty fixture as PluginLoader has only static methods so far

static constexpr double SAMPLE_RATE = 44000.0;

class TestLv2PluginLoader : public ::testing::Test
{
protected:
    TestLv2PluginLoader()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(TestLv2PluginLoader, TestLoadPlugin)
{
    const std::string plugin_URI = "http://lv2plug.in/plugins/eg-amp";

    PluginLoader loader;

    auto plugin_handle = loader.get_plugin_handle_from_URI(plugin_URI.c_str());

    ASSERT_NE(nullptr, plugin_handle);

    // Ilias TODO: populate feature_list.
    // It CAN be nullptr, for hosts which support no additional features.
    const LV2_Feature** feature_list = nullptr;

    loader.load_plugin(plugin_handle, SAMPLE_RATE, feature_list);

    auto instance = loader.getPluginInstance();

    ASSERT_NE(nullptr, instance);

    loader.close_plugin_instance(instance);

    ASSERT_EQ(nullptr, loader.getPluginInstance());
}