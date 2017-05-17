#include "gtest/gtest.h"

#include "library/vst2x_plugin_loader.cpp"
#include <cstdlib>

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
    // dlopen on Linux requires absolute paths if library is not on system paths already
    char* full_again_path = realpath("libagain.so", NULL);
    auto library_handle = PluginLoader::get_library_handle_for_plugin(full_again_path);
    EXPECT_NE(nullptr, library_handle);
    free(full_again_path);
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

