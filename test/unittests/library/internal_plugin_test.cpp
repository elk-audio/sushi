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

TEST_F(InternalPluginTest, TestParameterRegistration)
{
    EXPECT_TRUE(_module_under_test->register_bool_parameter("bool", "Bool", false));
    EXPECT_TRUE(_module_under_test->register_string_property("string", "String"));
    EXPECT_TRUE(_module_under_test->register_data_property("data", "Data"));
    EXPECT_TRUE(_module_under_test->register_int_parameter("int", "Int", 3,
                                                            new IntParameterPreProcessor(0, 10)));
    EXPECT_TRUE(_module_under_test->register_float_parameter("float", "Float", 5.0f,
                                                             new FloatParameterPreProcessor(0.0, 10.0)));

    /* Verify all parameter/properties were registered and their order match */
    auto parameter_list = _module_under_test->all_parameters();
    EXPECT_EQ(5u, parameter_list.size());

    EXPECT_EQ(5u, _module_under_test->_parameter_values.size());
    IntParameterValue* value;
    ASSERT_NO_FATAL_FAILURE(value = _module_under_test->_parameter_values[3].int_parameter_value());
    EXPECT_EQ(3, value->value());
}

TEST_F(InternalPluginTest, TestDuplicateParameterNames)
{
    auto test_param = _module_under_test->register_int_parameter("param_2", "Param 2", 1, new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(test_param);
    /*  Register another parameter with the same name and assert that we get a null pointer back */
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", 1, new BoolParameterPreProcessor(0, 1));
    EXPECT_FALSE(test_param_2);
}


TEST_F(InternalPluginTest, TestBoolParameterHandling)
{
    BoolParameterValue* value = _module_under_test->register_bool_parameter("param_1", "Param 1", false);
    EXPECT_TRUE(value);

    // access the parameter through its id and verify type and that you can set its value.
    EXPECT_EQ(ParameterType::BOOL, _module_under_test->parameter_from_name("param_1")->type());
    Event event = Event::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_TRUE(value->value());
}

TEST_F(InternalPluginTest, TestIntParameterHandling)
{
    IntParameterValue* value = _module_under_test->register_int_parameter("param_1", "Param 1", 0, new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(value);

    // access the parameter through its id and verify type and that you can set its value.
    EXPECT_EQ(ParameterType::INT, _module_under_test->parameter_from_name("param_1")->type());
    Event event = Event::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_FLOAT_EQ(6.0f, value->value());
}

TEST_F(InternalPluginTest, TestFloatParameterHandling)
{
    FloatParameterValue* value = _module_under_test->register_float_parameter("param_1", "Param 1", 1, new FloatParameterPreProcessor(0.0, 10.0));
    EXPECT_TRUE(value);

    // access the parameter through its id and verify type and that you can set its value.
    EXPECT_EQ(ParameterType::FLOAT, _module_under_test->parameter_from_name("param_1")->type());
    Event event = Event::make_parameter_change_event(0, 0, 0, 5);
    _module_under_test->process_event(event);
    EXPECT_EQ(5, value->value());
}