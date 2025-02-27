#include <fstream>

#include "gtest/gtest.h"
#include "library/parameter_dump.cpp"
#include "test_utils/control_mockup.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_COMPARISON_CALLS_NAME_RECURSIVELY

TEST(TestParameterDump, TestParameterDocumentGeneration)
{
    const std::string expected_result_str = "{\n    \"plugins\": [\n        {\n            \"name\": \"proc 1\",\n            \"label\": \"proc 1\",\n            \"processor_id\": 0,\n            \"parent_track_id\": 0,\n            \"parameters\": [\n                {\n                    \"name\": \"param 1\",\n                    \"label\": \"param 1\",\n                    \"osc_path\": \"/parameter/proc_1/param_1\",\n                    \"id\": 0\n                },\n                {\n                    \"name\": \"param 2\",\n                    \"label\": \"param 2\",\n                    \"osc_path\": \"/parameter/proc_1/param_2\",\n                    \"id\": 1\n                },\n                {\n                    \"name\": \"param 3\",\n                    \"label\": \"param 3\",\n                    \"osc_path\": \"/parameter/proc_1/param_3\",\n                    \"id\": 2\n                }\n            ]\n        },\n        {\n            \"name\": \"proc 2\",\n            \"label\": \"proc 2\",\n            \"processor_id\": 1,\n            \"parent_track_id\": 0,\n            \"parameters\": [\n                {\n                    \"name\": \"param 1\",\n                    \"label\": \"param 1\",\n                    \"osc_path\": \"/parameter/proc_2/param_1\",\n                    \"id\": 0\n                },\n                {\n                    \"name\": \"param 2\",\n                    \"label\": \"param 2\",\n                    \"osc_path\": \"/parameter/proc_2/param_2\",\n                    \"id\": 1\n                },\n                {\n                    \"name\": \"param 3\",\n                    \"label\": \"param 3\",\n                    \"osc_path\": \"/parameter/proc_2/param_3\",\n                    \"id\": 2\n                }\n            ]\n        },\n        {\n            \"name\": \"proc 1\",\n            \"label\": \"proc 1\",\n            \"processor_id\": 0,\n            \"parent_track_id\": 1,\n            \"parameters\": [\n                {\n                    \"name\": \"param 1\",\n                    \"label\": \"param 1\",\n                    \"osc_path\": \"/parameter/proc_1/param_1\",\n                    \"id\": 0\n                },\n                {\n                    \"name\": \"param 2\",\n                    \"label\": \"param 2\",\n                    \"osc_path\": \"/parameter/proc_1/param_2\",\n                    \"id\": 1\n                },\n                {\n                    \"name\": \"param 3\",\n                    \"label\": \"param 3\",\n                    \"osc_path\": \"/parameter/proc_1/param_3\",\n                    \"id\": 2\n                }\n            ]\n        },\n        {\n            \"name\": \"proc 2\",\n            \"label\": \"proc 2\",\n            \"processor_id\": 1,\n            \"parent_track_id\": 1,\n            \"parameters\": [\n                {\n                    \"name\": \"param 1\",\n                    \"label\": \"param 1\",\n                    \"osc_path\": \"/parameter/proc_2/param_1\",\n                    \"id\": 0\n                },\n                {\n                    \"name\": \"param 2\",\n                    \"label\": \"param 2\",\n                    \"osc_path\": \"/parameter/proc_2/param_2\",\n                    \"id\": 1\n                },\n                {\n                    \"name\": \"param 3\",\n                    \"label\": \"param 3\",\n                    \"osc_path\": \"/parameter/proc_2/param_3\",\n                    \"id\": 2\n                }\n            ]\n        }\n    ]\n}";
    rapidjson::Document expected_result;
    expected_result.Parse(expected_result_str.c_str());
    sushi::control::ControlMockup controller;

    rapidjson::Document result = sushi::generate_processor_parameter_document(&controller);

    ASSERT_TRUE(expected_result == result);
}

ELK_POP_WARNING