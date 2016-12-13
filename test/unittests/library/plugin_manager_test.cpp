#include "gtest/gtest.h"

#include "library/plugin_manager.h"

#define private public

using namespace sushi;

class TestPlugin : public StompBox
{
public:
    TestPlugin() {}

    StompBoxStatus init(const StompBoxConfig& /*configuration*/) override {return StompBoxStatus::OK;}

    std::string unique_id() const override {return "test_plugin";}

    void set_parameter(int /*parameter_id*/, float /*value*/) override {}

    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override
    {
        *out_buffer = *in_buffer;
    }
};

// A simple test case
/*TEST (SampleTest, SimpleTestCase) {
    ASSERT_TRUE (1);
}*/

class StompBoxManagerTest : public ::testing::Test
{
protected:
    StompBoxManagerTest()
    {
    }
    void SetUp()
    {
        _test_plugin = new TestPlugin;
        _module_under_test = new StompBoxManager(_test_plugin);
    }

    void TearDown()
    {
        // _test_plugin is managed by the StompBoxManager and should not be deleted
        delete(_module_under_test);
    }
    TestPlugin* _test_plugin;
    StompBoxManager* _module_under_test;
};


TEST_F(StompBoxManagerTest, TestInstanciation)
{
    EXPECT_TRUE(_module_under_test->instance());
    EXPECT_EQ("test_plugin", _module_under_test->instance()->unique_id());
}


TEST_F(StompBoxManagerTest, TestParameterHandling)
{
    BaseStompBoxParameter* test_param = _module_under_test->register_float_parameter("Param 1", "param_1", 1, 10, 0);

    // access the parameter through its id and verify type and that you can set its value.
    ASSERT_EQ(StompBoxParameterType::FLOAT, _module_under_test->get_parameter("param_1")->type());
    static_cast<FloatStompBoxParameter*>(_module_under_test->get_parameter("param_1"))->set(6.0f);
    EXPECT_FLOAT_EQ(6.0f, static_cast<FloatStompBoxParameter*>(test_param)->value());

    test_param = _module_under_test->register_int_parameter("Param 2", "param_2", 1, 10, 0);
    EXPECT_EQ(StompBoxParameterType::INT, test_param->type());

    test_param = _module_under_test->register_bool_parameter("Param 3", "param_3", true);
    EXPECT_EQ(StompBoxParameterType::BOOL, test_param->type());
}