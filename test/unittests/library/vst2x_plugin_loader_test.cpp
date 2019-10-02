#include "gtest/gtest.h"

#include "library/vst2x_plugin_loader.cpp"

using namespace sushi;
using namespace sushi::vst2;

// Empty fixture as PluginLoader has only static methods so far

class TestVst2xPluginLoader : public ::testing::Test
{
protected:
    TestVst2xPluginLoader()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

TEST_F(TestVst2xPluginLoader, TestLoadPlugin)
{
    // dlopen on Linux requires absolute paths if library is not on system paths already
    char* full_again_path = realpath("libvst2_test_plugin.so", NULL);
    auto library_handle = PluginLoader::get_library_handle_for_plugin(full_again_path);
    ASSERT_NE(nullptr, library_handle);
    free(full_again_path);
    auto plugin = PluginLoader::load_plugin(library_handle);

    // Check magic number
    EXPECT_EQ(kEffectMagic, plugin->magic);

    PluginLoader::close_library_handle(library_handle);
}

