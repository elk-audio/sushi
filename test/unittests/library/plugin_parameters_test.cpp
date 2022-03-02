#include "gtest/gtest.h"

#include "library/plugin_parameters.h"
#include "test_utils/test_utils.h"

using namespace sushi;

class TestdBToLinPreProcessor : public ::testing::Test
{
protected:
    TestdBToLinPreProcessor() {}

    dBToLinPreProcessor _module_under_test{-24.0f, 24.0f};
};

TEST_F(TestdBToLinPreProcessor, TestProcessing)
{
    EXPECT_NEAR(1.0, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.5f)), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(2.0, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.625f)), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(0.25, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.25f)), test_utils::DECIBEL_ERROR);
}

class TestLinTodBPreProcessor : public ::testing::Test
{
protected:
    TestLinTodBPreProcessor() {}

    LinTodBPreProcessor _module_under_test{0, 10.0f};
};

TEST_F(TestLinTodBPreProcessor, TestProcessing)
{
    EXPECT_NEAR(0.0f, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.1f)), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(6.02f, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.2f)), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(-12.04f, _module_under_test.process_to_plugin(_module_under_test.to_domain(0.025f)), test_utils::DECIBEL_ERROR);
}

/*
 * Templated testing is difficult since we want to test with different values for each type
 * Therefore we test each type separately.
 */

class TestParameter : public ::testing::Test
{
protected:
    TestParameter() {}

    uint8_t* TEST_DATA = new uint8_t[3];
    BlobData blob{3, TEST_DATA};

    FloatParameterDescriptor _module_under_test_float{"float_parameter",
                                                      "FloatParameter",
                                                      "fl",
                                                      -10.0f,
                                                      10.0f,
                                                      Direction::AUTOMATABLE,
                                                      new ParameterPreProcessor<float>(-10, 10)};

    IntParameterDescriptor _module_under_test_int{"int_parameter",
                                                  "IntParameter",
                                                  "int",
                                                  -10,
                                                  10,
                                                  Direction::AUTOMATABLE,
                                                  new ParameterPreProcessor<int>(-10, 10)};

    BoolParameterDescriptor _module_under_test_bool{"bool_parameter",
                                                    "BoolParameter",
                                                    "bool",
                                                    false,
                                                    true,
                                                    Direction::AUTOMATABLE,
                                                    new ParameterPreProcessor<bool>(0, 1)};

    StringPropertyDescriptor _module_under_test_string{"string_property", "String Property", ""};
    DataPropertyDescriptor _module_under_test_data{"data_property", "Data Property", "data"};
};

TEST_F(TestParameter, TestTypeNameAndLabel)
{
    EXPECT_EQ(ParameterType::BOOL, _module_under_test_bool.type());
    EXPECT_EQ(ParameterType::FLOAT, _module_under_test_float.type());
    EXPECT_EQ(ParameterType::INT, _module_under_test_int.type());
    EXPECT_EQ(ParameterType::STRING, _module_under_test_string.type());
    EXPECT_EQ(ParameterType::DATA, _module_under_test_data.type());

    EXPECT_EQ("bool_parameter", _module_under_test_bool.name());
    EXPECT_EQ("float_parameter", _module_under_test_float.name());
    EXPECT_EQ("int_parameter", _module_under_test_int.name());
    EXPECT_EQ("string_property", _module_under_test_string.name());
    EXPECT_EQ("data_property", _module_under_test_data.name());

    EXPECT_EQ("BoolParameter", _module_under_test_bool.label());
    EXPECT_EQ("FloatParameter", _module_under_test_float.label());
    EXPECT_EQ("IntParameter", _module_under_test_int.label());
    EXPECT_EQ("String Property", _module_under_test_string.label());
    EXPECT_EQ("Data Property", _module_under_test_data.label());

    EXPECT_EQ("bool", _module_under_test_bool.unit());
    EXPECT_EQ("fl", _module_under_test_float.unit());
    EXPECT_EQ("int", _module_under_test_int.unit());
    EXPECT_EQ("", _module_under_test_string.unit());
    EXPECT_EQ("data", _module_under_test_data.unit());
}

TEST(TestParameterValue, TestSet)
{
    dBToLinPreProcessor pre_processor(-6.0f, 6.0f);
    auto value = ParameterStorage::make_float_parameter_storage(nullptr, 0.0f, &pre_processor);
    /* Check correct defaults */
    EXPECT_EQ(ParameterType::FLOAT, value.float_parameter_value()->type());
    EXPECT_FLOAT_EQ(1.0f, value.float_parameter_value()->processed_value());
    EXPECT_FLOAT_EQ(0.0f, value.float_parameter_value()->domain_value());

    /* Test set */
    value.float_parameter_value()->set(pre_processor.to_normalized(6.0f));
    EXPECT_NEAR(2.0f, value.float_parameter_value()->processed_value(), 0.01f);
    EXPECT_FLOAT_EQ(6.0f, value.float_parameter_value()->domain_value());
}