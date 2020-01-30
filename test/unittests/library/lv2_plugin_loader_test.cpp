#include "gtest/gtest.h"

#include "library/lv2/lv2_symap.cpp"
#include "library/lv2/lv2_evbuf.cpp"
#include "library/lv2/lv2_control.cpp"
#include "library/lv2/lv2_plugin_loader.cpp"

using namespace sushi;
using namespace sushi::lv2;

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

    // Feature list can be nullptr, for hosts which support no additional features.
    // The eg-amp plugin depends on no extra features, so this is fine.
    const LV2_Feature** feature_list = nullptr;

    loader.load_plugin(plugin_handle, SAMPLE_RATE, feature_list);

    auto instance = loader.getModel()->get_plugin_instance();

    ASSERT_NE(nullptr, instance);

    loader.close_plugin_instance();

    ASSERT_EQ(nullptr, loader.getModel()->get_plugin_instance());
}