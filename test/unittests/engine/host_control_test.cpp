#include "gtest/gtest.h"

#define private public
#define protected public

#include "engine/host_control.h"

using namespace sushi;


class TestHostControl : public ::testing::Test
{
protected:
    TestHostControl()
    {
    }

    void SetUp()
    {}

    void TearDown()
    { }

    HostControl _module_under_test{nullptr, nullptr};
};


TEST_F(TestHostControl, TestPathWithoutBase)
{
    // Check that convert_plugin_path() is an identity
    // if no base path has been set

    std::string plugin_path{"/home/foo/bar/my_absolute_plugin.so"};
    ASSERT_EQ(plugin_path, _module_under_test.convert_plugin_path(plugin_path));
}

TEST_F(TestHostControl, TestPathConcatenation)
{
    _module_under_test.set_base_plugin_path("/home/foo/bar");
    ASSERT_EQ("/home/foo/bar/my_absolute_plugin.so", _module_under_test.convert_plugin_path("my_absolute_plugin.so"));
}
