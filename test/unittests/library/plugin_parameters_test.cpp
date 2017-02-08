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

    char* TEST_DATA{new char[5]};

    FloatStompBoxParameter _module_under_test_float{"float_parameter", "FloatParameter", 1.0f, new ParameterPreProcessor<float>(-10, 10)};
    IntStompBoxParameter _module_under_test_int{"int_parameter", "IntParameter", 0, new ParameterPreProcessor<int>(-10, 10)};
    BoolStompBoxParameter _module_under_test_bool{"bool_parameter", "BoolParameter", false, new ParameterPreProcessor<bool>(0, 1)};
    StringStompBoxParameter _module_under_test_string{"string_parameter", "String Parameter", new std::string("This is a string!")};
    DataStompBoxParameter _module_under_test_data{"data_parameter", "Data Parameter", TEST_DATA};
};

TEST_F(TestStompBoxParameter, TestTypeNameAndLabel)
{
    EXPECT_EQ(StompBoxParameterType::BOOL, _module_under_test_bool.type());
    EXPECT_EQ(StompBoxParameterType::FLOAT, _module_under_test_float.type());
    EXPECT_EQ(StompBoxParameterType::INT, _module_under_test_int.type());
    EXPECT_EQ(StompBoxParameterType::STRING, _module_under_test_string.type());
    EXPECT_EQ(StompBoxParameterType::DATA, _module_under_test_data.type());

    EXPECT_EQ("bool_parameter", _module_under_test_bool.id());
    EXPECT_EQ("float_parameter", _module_under_test_float.id());
    EXPECT_EQ("int_parameter", _module_under_test_int.id());
    EXPECT_EQ("string_parameter", _module_under_test_string.id());
    EXPECT_EQ("data_parameter", _module_under_test_data.id());

    EXPECT_EQ("BoolParameter", _module_under_test_bool.label());
    EXPECT_EQ("FloatParameter", _module_under_test_float.label());
    EXPECT_EQ("IntParameter", _module_under_test_int.label());
    EXPECT_EQ("String Parameter", _module_under_test_string.label());
    EXPECT_EQ("Data Parameter", _module_under_test_data.label());
}

TEST_F(TestStompBoxParameter, TestDefaultValue)
{
    EXPECT_FLOAT_EQ(1.0f, _module_under_test_float.value());
    EXPECT_EQ(0, _module_under_test_int.value());
    EXPECT_FALSE(_module_under_test_bool.value());
    EXPECT_EQ("This is a string!", *_module_under_test_string.value());

    EXPECT_FLOAT_EQ(1.0f, _module_under_test_float.raw_value());
    EXPECT_EQ(0, _module_under_test_int.raw_value());
    EXPECT_FALSE(_module_under_test_bool.raw_value());
}

TEST_F(TestStompBoxParameter, TestSet)
{
    _module_under_test_float.set(13.25f);
    EXPECT_FLOAT_EQ(10.0f, _module_under_test_float.value());
    EXPECT_FLOAT_EQ(13.25f, _module_under_test_float.raw_value());

    _module_under_test_int.set(5);
    EXPECT_EQ(5, _module_under_test_int.value());
    EXPECT_EQ(5, _module_under_test_int.raw_value());

    _module_under_test_bool.set(true);
    EXPECT_TRUE(_module_under_test_bool.value());
    EXPECT_TRUE(_module_under_test_bool.raw_value());

    std::string* new_value = new std::string("New String!");
    _module_under_test_string.set(new_value);
    EXPECT_EQ("New String!", *_module_under_test_string.value());
}

TEST_F(TestStompBoxParameter, TestAsString)
{
    EXPECT_EQ("1.000000", _module_under_test_float.as_string());
    EXPECT_EQ("0", _module_under_test_int.as_string());
    EXPECT_EQ("False", _module_under_test_bool.as_string());
    EXPECT_EQ("This is a string!", _module_under_test_string.as_string());
    EXPECT_EQ("Binary data", _module_under_test_data.as_string());
    // Test that we get the correct format even when we call as_string() when
    // The parameter is accessed as an instance of BaseStompBoxParameter
    EXPECT_EQ("1.000000", static_cast<BaseStompBoxParameter*>(&_module_under_test_float)->as_string());
    EXPECT_EQ("False", static_cast<BaseStompBoxParameter*>(&_module_under_test_bool)->as_string());

}