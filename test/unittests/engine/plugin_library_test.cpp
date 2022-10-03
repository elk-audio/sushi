#include "gtest/gtest.h"

#define private public
#define protected public

#include "engine/plugin_library.cpp"

using namespace sushi;


class TestPluginLibrary : public ::testing::Test
{
protected:
    TestPluginLibrary()
    {
    }

    void SetUp()
    {}

    void TearDown()
    { }

    engine::PluginLibrary _module_under_test;
};


TEST_F(TestPluginLibrary, TestPathWithoutBase)
{
    // Check that convert_plugin_path() is an identity
    // if no base path has been set

    std::string plugin_path{"/home/foo/bar/my_absolute_plugin.so"};
    ASSERT_EQ(plugin_path, _module_under_test.convert_plugin_path(plugin_path));
}

TEST_F(TestPluginLibrary, TestPathConcatenation)
{
    _module_under_test.set_base_plugin_path("/home/foo/bar");
    ASSERT_EQ("/home/foo/bar/my_absolute_plugin.so", _module_under_test.convert_plugin_path("my_absolute_plugin.so"));
}

