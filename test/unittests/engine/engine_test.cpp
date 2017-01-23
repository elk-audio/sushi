
#include <algorithm>
#include <fstream>
#include <iostream>

#include "gtest/gtest.h"
#include "test_utils.h"

#define private public
#define protected public

#include "engine/engine.cpp"
#include "library/mind_allocator.cpp"

using ::testing::internal::posix::GetEnv;


constexpr unsigned int SAMPLE_RATE = 44000;
using namespace sushi;
using namespace sushi::engine;

/*
* Enginge tests
*/
class TestEngine : public ::testing::Test
{
protected:
    TestEngine()
    {
    }

    void SetUp()
    {
        _module_under_test = new AudioEngine(SAMPLE_RATE);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    AudioEngine* _module_under_test;
};

TEST_F(TestEngine, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

/*
 * Test that 1:s in gives 1:s out
 */
TEST_F(TestEngine, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::assert_buffer_value(1.0f, in_buffer);


    _module_under_test->process_chunk(&in_buffer, &out_buffer);

    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestEngine, TestInitFromJSON)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }

    // Initialize with a file cointaining 0.5 on both channels
    std::string test_config_file(test_data_dir);
    test_config_file.append("/config.json");

    std::ifstream file(test_config_file);
    Json::Value config;
    Json::Reader reader;
    bool parse_ok = reader.parse(file, config, false);
    if (!parse_ok)
    {
        EXPECT_TRUE(false) << "Error parsing JSON config file\n";
    }

    _module_under_test->init_from_json_array(config["stompbox_chains"]);

    auto chain_l = &_module_under_test->_audio_graph[0]._chain;
    auto chain_r = &_module_under_test->_audio_graph[1]._chain;

    ASSERT_EQ(chain_l->size(), 3u);
    ASSERT_EQ(chain_r->size(), 3u);

    /* TODO - Is this casting a good idea */
    ASSERT_EQ(static_cast<StompBoxManager*>(chain_l->at(0))->instance()->unique_id(), "sushi.testing.passthrough");
    ASSERT_EQ(static_cast<StompBoxManager*>(chain_l->at(1))->instance()->unique_id(), "sushi.testing.gain");
    ASSERT_EQ(static_cast<StompBoxManager*>(chain_l->at(2))->instance()->unique_id(), "sushi.testing.gain");

    ASSERT_EQ(static_cast<StompBoxManager*>(chain_r->at(0))->instance()->unique_id(), "sushi.testing.gain");
    ASSERT_EQ(static_cast<StompBoxManager*>(chain_r->at(1))->instance()->unique_id(), "sushi.testing.passthrough");
    ASSERT_EQ(static_cast<StompBoxManager*>(chain_r->at(2))->instance()->unique_id(), "sushi.testing.gain");
}
