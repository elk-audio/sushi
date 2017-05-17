
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

Json::Value read_json_config(const std::string& file_name)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }
    std::string test_config_file(test_data_dir);
    test_config_file.append("/" + file_name);

    std::ifstream file(test_config_file);
    Json::Value config;
    Json::Reader reader;
    bool parse_ok = reader.parse(file, config, false);
    if (!parse_ok)
    {
        EXPECT_TRUE(false) << "Error parsing JSON config file\n";
    }
    return config;
};

/*
* Engine tests
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
    /* Add a plugin chain since the engine by default doesn't have any */
    PluginChain chain;
    _module_under_test->_audio_graph.push_back(&chain);

    /* Run tests */
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::assert_buffer_value(1.0f, in_buffer);

    _module_under_test->process_chunk(&in_buffer, &out_buffer);

    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(TestEngine, TestInitFromJSON)
{
    Json::Value config = read_json_config("config.json");
    ASSERT_FALSE(config.empty());
    EngineReturnStatus status = _module_under_test->init_chains_from_json_array(config["stompbox_chains"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    EXPECT_EQ(2, _module_under_test->_audio_graph[0]->input_channels());
    EXPECT_EQ(2, _module_under_test->_audio_graph[0]->output_channels());
    EXPECT_EQ(1, _module_under_test->_audio_graph[1]->input_channels());
    EXPECT_EQ(1, _module_under_test->_audio_graph[1]->output_channels());

    auto chain_l = &_module_under_test->_audio_graph[0]->_chain;
    auto chain_r = &_module_under_test->_audio_graph[1]->_chain;

    ASSERT_EQ(chain_l->size(), 3u);

    ASSERT_EQ(chain_r->size(), 3u);
    EXPECT_EQ(1, _module_under_test->_audio_graph[1]->input_channels());

    /* TODO - Is this casting a good idea */
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(0))->name(), "passthrough_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(1))->name(), "gain_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(2))->name(), "equalizer_0_l");

    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(0))->name(), "gain_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(1))->name(), "passthrough_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(2))->name(), "gain_1_r");
}

TEST_F(TestEngine, TestInitMidiFromJSON)
{
    Json::Value config = read_json_config("config.json");
    ASSERT_FALSE(config.empty());
    _module_under_test->set_midi_input_ports(1);
    EngineReturnStatus status = _module_under_test->init_chains_from_json_array(config["stompbox_chains"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _module_under_test->init_midi_from_json_array(config["midi"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ASSERT_EQ(1u, _module_under_test->_midi_dispatcher._kb_routes.size());
    ASSERT_EQ(1u, _module_under_test->_midi_dispatcher._cc_routes.size());
}

TEST_F(TestEngine, TestUidNameMapping)
{
    Json::Value config = read_json_config("config.json");
    ASSERT_FALSE(config.empty());
    EngineReturnStatus status = _module_under_test->init_chains_from_json_array(config["stompbox_chains"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ObjectId id;
    std::tie(status, id) = _module_under_test->processor_id_from_name("gain_0_l");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    std::string name;
    std::tie(status, name) = _module_under_test->processor_name_from_id(id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ("gain_0_l", name);

    /* Test with name/id that doesn't match any processors */
    std::tie(status, id) = _module_under_test->processor_id_from_name("not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_STOMPBOX_UID, status);
    std::tie(status, name) = _module_under_test->processor_name_from_id(123456);
    ASSERT_EQ(EngineReturnStatus::INVALID_STOMPBOX_UID, status);
}

TEST_F(TestEngine, TestParameterLookup)
{
    Json::Value config = read_json_config("config.json");
    ASSERT_FALSE(config.empty());
    EngineReturnStatus status = _module_under_test->init_chains_from_json_array(config["stompbox_chains"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ObjectId id;
    std::tie(status, id) = _module_under_test->parameter_id_from_name("equalizer_0_l", "q");
    ASSERT_EQ(EngineReturnStatus::OK, status);

    std::tie(status, id) = _module_under_test->parameter_id_from_name("not_found", "gain");
    ASSERT_EQ(EngineReturnStatus::INVALID_STOMPBOX_UID, status);

    std::tie(status, id) = _module_under_test->parameter_id_from_name("gain_0_l", "not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_PARAMETER_UID, status);
}