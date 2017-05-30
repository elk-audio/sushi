#include "gtest/gtest.h"

#include "library/plugin_parameters.h"
#include "test_utils.h"

#define private public

using namespace sushi;

class TestParameterPreProcessor : public ::testing::Test
{
protected:
    TestParameterPreProcessor() {}

    ParameterPreProcessor<float> _module_under_test{-10, 10};
};

TEST_F(TestParameterPreProcessor, TestClipping)
{
    EXPECT_FLOAT_EQ(1.0f, _module_under_test.process(1.0f));
    EXPECT_FLOAT_EQ(10.0f, _module_under_test.process(10.0f));
    EXPECT_FLOAT_EQ(-4.0, _module_under_test.process(-4.0f));

    EXPECT_FLOAT_EQ(-10.0, _module_under_test.process(-14.0f));
    EXPECT_FLOAT_EQ(10.0, _module_under_test.process(267890.5f));
}


class TestdBToLinPreProcessor : public ::testing::Test
{
protected:
    TestdBToLinPreProcessor() {}

    dBToLinPreProcessor _module_under_test{-24.0f, 24.0f};
};

TEST_F(TestdBToLinPreProcessor, TestProcessing)
{
    EXPECT_NEAR(1.0, _module_under_test.process(0.0f), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(2.0, _module_under_test.process(6.0f), test_utils::DECIBEL_ERROR);
    EXPECT_NEAR(0.25, _module_under_test.process(-12.0f), test_utils::DECIBEL_ERROR);
}

/*
 * Templated testing is difficult since we want to test with different values for each type
 * Therefore we test each type separately.
 */

class TestStompBoxParameter : public ::testing::Test
{
protected:
    TestStompBoxParameter() {}

    uint8_t* TEST_DATA = new uint8_t[3];
    BlobData blob{3, TEST_DATA};

    FloatStompBoxParameter _module_under_test_float{"float_parameter", "FloatParameter", new ParameterPreProcessor<float>(-10, 10)};
    IntStompBoxParameter _module_under_test_int{"int_parameter", "IntParameter", new ParameterPreProcessor<int>(-10, 10)};
    BoolStompBoxParameter _module_under_test_bool{"bool_parameter", "BoolParameter", new ParameterPreProcessor<bool>(0, 1)};
    StringStompBoxProperty _module_under_test_string{"string_parameter", "String Parameter"};
    DataStompBoxProperty _module_under_test_data{"data_parameter", "Data Parameter"};
};

TEST_F(TestStompBoxParameter, TestTypeNameAndLabel)
{
    EXPECT_EQ(StompBoxParameterType::BOOL, _module_under_test_bool.type());
    EXPECT_EQ(StompBoxParameterType::FLOAT, _module_under_test_float.type());
    EXPECT_EQ(StompBoxParameterType::INT, _module_under_test_int.type());
    EXPECT_EQ(StompBoxParameterType::STRING, _module_under_test_string.type());
    EXPECT_EQ(StompBoxParameterType::DATA, _module_under_test_data.type());

    EXPECT_EQ("bool_parameter", _module_under_test_bool.name());
    EXPECT_EQ("float_parameter", _module_under_test_float.name());
    EXPECT_EQ("int_parameter", _module_under_test_int.name());
    EXPECT_EQ("string_parameter", _module_under_test_string.name());
    EXPECT_EQ("data_parameter", _module_under_test_data.name());

    EXPECT_EQ("BoolParameter", _module_under_test_bool.label());
    EXPECT_EQ("FloatParameter", _module_under_test_float.label());
    EXPECT_EQ("IntParameter", _module_under_test_int.label());
    EXPECT_EQ("String Parameter", _module_under_test_string.label());
    EXPECT_EQ("Data Parameter", _module_under_test_data.label());
}

TEST(TestParameterValue, TestSet)
{
    dBToLinPreProcessor pre_processor(-6.0f, 6.0f);
    auto value = ParameterStorage::make_float_parameter_storage(nullptr, 0, &pre_processor);
    /* Check correct defaults */
    EXPECT_EQ(StompBoxParameterType::FLOAT, value.float_parameter_value()->type());
    EXPECT_FLOAT_EQ(1.0f, value.float_parameter_value()->value());
    EXPECT_FLOAT_EQ(0.0f, value.float_parameter_value()->raw_value());

    /* Test set */
    value.float_parameter_value()->set(6.0f);
    EXPECT_NEAR(2.0f, value.float_parameter_value()->value(), 0.01f);
    EXPECT_FLOAT_EQ(6.0f, value.float_parameter_value()->raw_value());
}