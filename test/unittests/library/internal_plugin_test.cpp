#include <tuple>

#include "gtest/gtest.h"

#define private public
#include "library/internal_plugin.cpp"
#undef private

#include "test_utils/host_control_mockup.h"
#include "test_utils/test_utils.h"


using namespace sushi;

class TestPlugin : public InternalPlugin
{
public:
    TestPlugin(HostControl host_control) : InternalPlugin(host_control)
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
        _module_under_test = new TestPlugin(_host_control.make_host_control_mockup());
    }

    void TearDown()
    {
        delete(_module_under_test);
    }
    HostControlMockup _host_control;
    InternalPlugin* _module_under_test;
};


TEST_F(InternalPluginTest, TestInstanciation)
{
    EXPECT_EQ("test_plugin", _module_under_test->name());
}

TEST_F(InternalPluginTest, TestParameterRegistration)
{
    EXPECT_TRUE(_module_under_test->register_bool_parameter("bool", "Bool", "bool", false));
    EXPECT_TRUE(_module_under_test->register_string_property("string", "String", ""));
    EXPECT_TRUE(_module_under_test->register_data_property("data", "Data", ""));
    EXPECT_TRUE(_module_under_test->register_int_parameter("int", "Int", "numbers", 3, 0, 10,
                                                            new IntParameterPreProcessor(0, 10)));
    EXPECT_TRUE(_module_under_test->register_float_parameter("float", "Float", "fl", 5.0f, 0.0f, 10.0f,
                                                             new FloatParameterPreProcessor(0.0, 10.0)));

    // Verify all parameter/properties were registered and their order match
    auto parameter_list = _module_under_test->all_parameters();
    EXPECT_EQ(5u, parameter_list.size());

    EXPECT_EQ(5u, _module_under_test->_parameter_values.size());
    IntParameterValue* value = nullptr;
    ASSERT_NO_FATAL_FAILURE(value = _module_under_test->_parameter_values[3].int_parameter_value());
    EXPECT_EQ(3, value->value());
}

TEST_F(InternalPluginTest, TestDuplicateParameterNames)
{
    auto test_param = _module_under_test->register_int_parameter("param_2", "Param 2", "", 1, 0, 10,
                                                                 new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(test_param);
    //  Register another parameter with the same name and assert that we get a null pointer back
    auto test_param_2 = _module_under_test->register_bool_parameter("param_2", "Param 2", "", false);
    EXPECT_FALSE(test_param_2);
}

TEST_F(InternalPluginTest, TestBoolParameterHandling)
{
    BoolParameterValue* value = _module_under_test->register_bool_parameter("param_1", "Param 1", "", false);
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::BOOL, _module_under_test->parameter_from_name("param_1")->type());
    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_TRUE(value->value());
    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(1.0f, ext_value);
    auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}

TEST_F(InternalPluginTest, TestIntParameterHandling)
{
    IntParameterValue* value = _module_under_test->register_int_parameter("param_1", "Param 1", "", 0, 0, 10,
                                                                          new IntParameterPreProcessor(0, 10));
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::INT, _module_under_test->parameter_from_name("param_1")->type());
    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 6.0f);
    _module_under_test->process_event(event);
    EXPECT_FLOAT_EQ(6.0f, value->value());
    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(6.0f, ext_value);
    auto [status_1, norm_value] = _module_under_test->parameter_value_normalised(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status_1);
    EXPECT_FLOAT_EQ(0.6f, norm_value);
    auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}

TEST_F(InternalPluginTest, TestFloatParameterHandling)
{
    FloatParameterValue* value = _module_under_test->register_float_parameter("param_1", "Param 1", "", 1.0f, 0.0f, 10.f,
                                                                              new FloatParameterPreProcessor(0.0, 10.0));
    EXPECT_TRUE(value);

    // Access the parameter through its id, verify type and that you can set its value.
    EXPECT_EQ(ParameterType::FLOAT, _module_under_test->parameter_from_name("param_1")->type());
    RtEvent event = RtEvent::make_parameter_change_event(0, 0, 0, 5);
    _module_under_test->process_event(event);
    EXPECT_EQ(5, value->value());
    // Access the parameter from the external interface
    auto [status, ext_value] = _module_under_test->parameter_value(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(5.0f, ext_value);
    auto [status_1, norm_value] = _module_under_test->parameter_value_normalised(value->descriptor()->id());
    EXPECT_EQ(ProcessorReturnCode::OK, status_1);
    EXPECT_FLOAT_EQ(0.5f, norm_value);
    [[maybe_unused]] auto [err_status, unused_value] = _module_under_test->parameter_value(45);
    EXPECT_EQ(ProcessorReturnCode::PARAMETER_NOT_FOUND, err_status);

    DECLARE_UNUSED(unused_value);
}
