#include "gtest/gtest.h"

#define private public

#include "library/internal_plugin.h"

using namespace sushi;

class TestPlugin : public InternalPlugin
{
public:
    TestPlugin() {}

    const std::string unique_id() override { return "test_plugin"; }

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override
    {
        out_buffer = in_buffer;
    }
};


class StompBoxManagerTest : public ::testing::Test
{
protected:
    StompBoxManagerTest()
    {
    }
    void SetUp()
    {
        _module_under_test = new TestPlugin;
    }

    void TearDown()
    {
        delete(_module_under_test);
    }
    InternalPlugin* _module_under_test;
};


TEST_F(StompBoxManagerTest, TestInstanciation)
{
    EXPECT_EQ("test_plugin", _module_under_test->unique_id());
}


TEST_F(StompBoxManagerTest, TestParameterHandlingViaEvents)
{
    BaseStompBoxParameter* test_param = _module_under_test->register_float_parameter("param_1", "Param 1", 1, new FloatParameterPreProcessor(0.0, 10.0));

    // access the parameter through its id and verify type and that you can set its value.
    ASSERT_EQ(StompBoxParameterType::FLOAT, _module_under_test->get_parameter("param_1")->type());
    ParameterChangeEvent event(EventType::FLOAT_PARAMETER_CHANGE, "processor", 0, "param_1", 6.0f);
    _module_under_test->process_event(&event);
    EXPECT_FLOAT_EQ(6.0f, static_cast<FloatStompBoxParameter*>(test_param)->value());

    test_param = _module_under_test->register_int_parameter("param_2", "Param 2", 1, new IntParameterPreProcessor(0, 10));
    EXPECT_EQ(StompBoxParameterType::INT, test_param->type());

    test_param = _module_under_test->register_bool_parameter("param_3", "Param 3", true);
    EXPECT_EQ(StompBoxParameterType::BOOL, test_param->type());

    test_param = _module_under_test->register_string_parameter("param_4", "Param 4", "4");
    ASSERT_EQ(StompBoxParameterType::STRING, _module_under_test->get_parameter("param_4")->type());
    std::string* str_value = new std::string("5");
    StringParameterChangeEvent event_4("processor", 0, "param_4", str_value);
    _module_under_test->process_event(&event_4);
    EXPECT_EQ("5", *static_cast<StringStompBoxParameter*>(_module_under_test->get_parameter("param_4"))->value());

    //test that an unknown parameter returns a null pointer
    EXPECT_EQ(nullptr, _module_under_test->get_parameter("not_registered"));
}