
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

TEST_F(TestEngine, TestProcessorExist)
{
    Json::Value config = read_json_config("config.json");
    ASSERT_FALSE(config.empty());
    EngineReturnStatus status = _module_under_test->init_chains_from_json_array(config["stompbox_chains"]);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    /* Test by passing processor names as arguments */
    ASSERT_TRUE(_module_under_test->_processor_exists("passthrough_0_l"));
    ASSERT_TRUE(_module_under_test->_processor_exists("gain_1_r"));

    /* Test by passing processor names as arguments */
    ObjectId id;
    std::tie(status, id) = _module_under_test->processor_id_from_name("passthrough_0_l");
    ASSERT_TRUE(_module_under_test->_processor_exists(id));
    std::tie(status, id) = _module_under_test->processor_id_from_name("gain_1_r");
    ASSERT_TRUE(_module_under_test->_processor_exists(id));
    ASSERT_EQ(EngineReturnStatus::OK, status);

    /* Test by passing invalid names */
    ASSERT_FALSE(_module_under_test->_processor_exists("not_found"));
    ASSERT_FALSE(_module_under_test->_processor_exists(0u));
}

TEST_F(TestEngine, TestCreateEmptyPluginChain)
{
    /* Test chain creation and existance in processor lists */
    ObjectId id;
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    std::tie(status,id) = _module_under_test->processor_id_from_name("left");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_EQ(_module_under_test->_audio_graph.size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->name(),"left");

    /* Test if pluginchain can be created if the "name" already exists */
    status = _module_under_test->create_plugin_chain("left",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);

    /* Test if invalid number of channels or empty name is specified */
    status = _module_under_test->create_plugin_chain("left",3);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
    status = _module_under_test->create_plugin_chain("left",0);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
    status = _module_under_test->create_plugin_chain("",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);
}

TEST_F(TestEngine, TestCorrectAddPlugin)
{
    ObjectId id;
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);

    status = _module_under_test->add_plugin_to_chain("left","sushi.testing.passthrough","passthrough_0_l");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    std::tie(status,id) = _module_under_test->processor_id_from_name("passthrough_0_l");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    /* Check if plugin exists in the chain */
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain.size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[0]->name(),"passthrough_0_l");

    /* Add another plugin to same chain */
    status = _module_under_test->add_plugin_to_chain("left","sushi.testing.gain","gain_0_r");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    std::tie(status,id) = _module_under_test->processor_id_from_name("gain_0_r");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain.size(),2u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[1]->name(),"gain_0_r");
}

TEST_F(TestEngine, TestIncorrectAddPlugin)
{
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);

    /* Check if bad parameters are passed */
    status = _module_under_test->add_plugin_to_chain("","sushi.testing.passthrough","plugin_name");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);
    status = _module_under_test->add_plugin_to_chain("left","sushi.testing.passthrough","");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_STOMPBOX_NAME);
    status = _module_under_test->add_plugin_to_chain("left","dummyuid","dummyname");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_STOMPBOX_UID);

    /* Test adding plugin to non existing chain */
    status = _module_under_test->add_plugin_to_chain("dummychain","sushi.testing.passthrough","dummyname");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);

    /* Test adding plugin with already existing unique name */
    status = _module_under_test->add_plugin_to_chain("left","sushi.testing.gain","gain_0_r");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    status = _module_under_test->add_plugin_to_chain("left","sushi.testing.gain","gain_0_r");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_STOMPBOX_NAME);
}