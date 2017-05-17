#include "gtest/gtest.h"

#include "library/vst2x_plugin_loader.cpp"

using namespace sushi;
using namespace sushi::vst2;

// Empty fixture as PluginLoader has only static methods so far

class TestPluginLoader : public ::testing::Test
{
protected:
    TestPluginLoader()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(TestPluginLoader, TestLoadPLugin)
{

    std::string again_binary_filename("libagain.so");

    auto library_handle = PluginLoader::get_library_handle_for_plugin(again_binary_filename);
    EXPECT_NE(nullptr, library_handle);
    auto plugin = PluginLoader::load_plugin(library_handle);

    char effect_name[256] = {0};
    char vendor_string[256] = {0};
    char product_string[256] = {0};

    plugin->dispatcher(plugin, effGetEffectName, 0, 0, effect_name, 0);
    plugin->dispatcher(plugin, effGetVendorString, 0, 0, vendor_string, 0);
    plugin->dispatcher(plugin, effGetProductString, 0, 0, product_string, 0);

    EXPECT_EQ("Gain", std::string(effect_name));
    EXPECT_EQ("Gain", std::string(product_string));
    EXPECT_EQ("Steinberg Media Technologies", std::string(vendor_string));

    PluginLoader::close_library_handle(library_handle);
}

