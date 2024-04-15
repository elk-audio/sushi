#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "engine/plugin_library.cpp"

using namespace sushi;
using namespace sushi::internal;

#ifdef __APPLE__
constexpr auto PLUGIN_PATH = "/home/foo/bar/my_absolute_plugin.so";
constexpr auto INEXISTENT_PATH = "/home/foo/bar";
#elif defined(_MSC_VER)
constexpr auto PLUGIN_PATH = R"(C:\home\foo\bar\my_absolute_plugin.so)";
constexpr auto INEXISTENT_PATH = R"(C:\home\foo\bar)";
#endif

class TestPluginLibrary : public ::testing::Test
{
protected:
    TestPluginLibrary() = default;

    engine::PluginLibrary _module_under_test;
};


TEST_F(TestPluginLibrary, TestAbsolutePath)
{
    // Check that to_absolute_path() is an identity
    // if the path is already absolute

    std::string plugin_path{PLUGIN_PATH};
    ASSERT_EQ(plugin_path, _module_under_test.to_absolute_path(plugin_path));
}

TEST_F(TestPluginLibrary, TestEmptyPath)
{
    // Make sure we don't concatenate empty path with the base path

    ASSERT_EQ("", _module_under_test.to_absolute_path(""));
}

TEST_F(TestPluginLibrary, TestPathConcatenation)
{
    _module_under_test.set_base_plugin_path(INEXISTENT_PATH);
    ASSERT_EQ(PLUGIN_PATH, _module_under_test.to_absolute_path("my_absolute_plugin.so"));
}

