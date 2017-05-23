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
    // dlopen on Linux requires absolute paths if library is not on system paths already
    char* full_again_path = realpath("libagain.so", NULL);
    auto library_handle = PluginLoader::get_library_handle_for_plugin(full_again_path);
    EXPECT_NE(nullptr, library_handle);
    free(full_again_path);
    auto plugin = PluginLoader::load_plugin(library_handle);

    // Check magic number
    EXPECT_EQ(plugin->magic, kEffectMagic);

    PluginLoader::close_library_handle(library_handle);
}

