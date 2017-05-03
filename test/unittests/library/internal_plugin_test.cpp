#include "gtest/gtest.h"

#define private public

#include "library/internal_plugin.h"

using namespace sushi;

class TestPlugin : public InternalPlugin
{
public:
    TestPlugin()
    {
        set_name("test_plugin");
    }

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override
    {
        out_buffer = in_buffer;
    }
};


class InternalPluginTest : public ::testing::Test
{
protected:
    InternalPluginTest()
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


TEST_F(InternalPluginTest, TestInstanciation)
{
    EXPECT_EQ("test_plugin", _module_under_test->name());
}


TEST_F(InternalPluginTest, TestDuplicateParameterNames)
{
    auto test_param = _module_under_test->register_int_parameter("param_2", "Param 2", 1, new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(test_param);
    /*  Register another parameter with the same name and assert that we get a null pointer back */
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", 1, new BoolParameterPreProcessor(0, 1));
    EXPECT_FALSE(test_param_2);
}

TEST_F(InternalPluginTest, TestParameterHandlingViaEvents)
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


TEST_F(InternalPluginTest, TestParameterId)
{
    auto test_param = _module_under_test->register_int_parameter("param_1", "Param 1", 1, new IntParameterPreProcessor(0, 10));
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", 1, new BoolParameterPreProcessor(0, 1));

    uint32_t id_1;
    uint32_t id_2;
    ProcessorReturnCode status;
    /* Register 2 parameters and check that the ids match */
    std::tie(status, id_1) = _module_under_test->parameter_id_from_name("param_1");
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(id_1, test_param->id());

    std::tie(status, id_2) = _module_under_test->parameter_id_from_name("param_2");
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(id_2, test_param_2->id());
    EXPECT_NE(id_1, id_2);

    /* Assert that we get an error status for a non-existent parameter */
    std::tie(status, id_2) = _module_under_test->parameter_id_from_name("param_3");
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, status);
}