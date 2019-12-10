#include "gtest/gtest.h"
#include "library/parameter_dump.cpp"
#include "test_utils/parameter_dump_control_mockup.h"

TEST(ParameterDumpTest, TestParameterDump)
{
    std::string test_file_path = "/tmp/parameter_dump_test.json";
    sushi::ext::ControlMockup controller;

    ASSERT_EQ(sushi::dump_engine_processor_parameters(&controller, test_file_path),0);
}