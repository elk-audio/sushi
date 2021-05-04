#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"

#define private public

#include "plugins/send_return_factory.cpp"
#include "plugins/send_plugin.cpp"
#include "plugins/return_plugin.cpp"

using namespace sushi;
using namespace sushi::send_plugin;
using namespace sushi::return_plugin;

constexpr float TEST_SAMPLERATE = 44100;

TEST(TestSendReturnFactory, TestPluginCreation)
{
    SendReturnFactory factory;
    HostControlMockup host_control_mockup;
    auto host_ctrl = host_control_mockup.make_host_control_mockup(TEST_SAMPLERATE);
    PluginInfo info{.uid = "sushi.testing.send", .path = "", .type = PluginType::INTERNAL};

    auto [send_status, send_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, send_status);
    ASSERT_GT(send_plugin.use_count(), 0);
    ASSERT_GT(send_plugin->id(), 0);

    info.uid = "sushi.testing.return";
    auto [ret_status, return_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_EQ(ProcessorReturnCode::OK, ret_status);
    ASSERT_GT(return_plugin.use_count(), 0);
    ASSERT_GT(return_plugin->id(), 0);

    // Negative test
    info.uid = "sushi.testing.aux_";
    auto [error_status, error_plugin] = factory.new_instance(info, host_ctrl, TEST_SAMPLERATE);
    ASSERT_NE(ProcessorReturnCode::OK, error_status);
    ASSERT_FALSE(error_plugin);
}


class TestSendReturnPlugins : public ::testing::Test
{
protected:
    TestSendReturnPlugins()
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
    SendReturnFactory _factory;
};

TEST_F(TestSendReturnPlugins, TestPluginCreation)
{}