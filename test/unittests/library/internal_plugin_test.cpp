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
    auto value_float = _module_under_test->register_float_parameter("param_1", "Param 1", 1, new FloatParameterPreProcessor(0.0, 10.0));
    EXPECT_TRUE(value_float);

    // access the parameter through its id and verify type and that you can set its value.
    EXPECT_EQ(ParameterType::FLOAT, _module_under_test->get_parameter("param_1")->type());
    Event event = Event::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_FLOAT_EQ(6.0f, value_float->value());

    auto value_int = _module_under_test->register_int_parameter("param_2", "Param 2", 1, new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(value_int);
    EXPECT_EQ(ParameterType::INT, _module_under_test->get_parameter("param_2")->type());
    // TODO - test cases for all parameter changes

    auto value_bool = _module_under_test->register_bool_parameter("param_3", "Param 3", true);
    EXPECT_TRUE(value_bool);
    EXPECT_EQ(ParameterType::BOOL, _module_under_test->get_parameter("param_3")->type());

    bool ok = _module_under_test->register_string_property("param_4", "Param 4");
    EXPECT_EQ(ParameterType::STRING, _module_under_test->get_parameter("param_4")->type());

    /* std::string* str_value = new std::string("5");
    Event event_4 = Event::make_string_parameter_change_event(0, 0, 3, str_value);
    _module_under_test->process_event(event_4);*/
    //EXPECT_EQ("5", *static_cast<StringPropertyDescriptor*>(_module_under_test->get_parameter("param_4"))->value());

    //test that an unknown parameter returns a null pointer
    EXPECT_EQ(nullptr, _module_under_test->get_parameter("not_registered"));
}


TEST_F(InternalPluginTest, TestParameterId)
{
    auto test_param = _module_under_test->register_int_parameter("param_1", "Param 1", 1, new IntParameterPreProcessor(0, 10));
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", 1, new BoolParameterPreProcessor(0, 1));

    ObjectId id_1;
    ObjectId id_2;
    ProcessorReturnCode status;
    /* Register 2 parameters and check that the ids match */
    std::tie(status, id_1) = _module_under_test->parameter_id_from_name("param_1");
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(id_1, 0);

    std::tie(status, id_2) = _module_under_test->parameter_id_from_name("param_2");
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ(id_2, 1);
    EXPECT_NE(id_1, id_2);

    /* Assert that we get an error status for a non-existent parameter */
    std::tie(status, id_2) = _module_under_test->parameter_id_from_name("param_3");
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, status);
}