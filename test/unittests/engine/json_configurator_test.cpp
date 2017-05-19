#include "gtest/gtest.h"
#include "test_utils.h"
#include <fstream>
#include <iostream>

#define private public
#define protected public

#include "engine/engine.h"
#include "engine/json_configurator.cpp"


constexpr unsigned int SAMPLE_RATE = 44000;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::jsonconfig;

class TestJsonConfigurer : public ::testing::Test
{
protected:
    TestJsonConfigurer()
    {
    }

    void SetUp()
    {
        _engine = new AudioEngine(SAMPLE_RATE);
        _configurator = new JsonConfigurator(_engine);
        _status = JsonConfigReturnStatus::OK;
        _path = test_utils::get_data_dir_path();
        _path.append("config.json");
        _config = Json::Value::null;
        _plugin_chain = Json::Value::null;
    }

    void TearDown()
    {
        delete _engine;
        delete _configurator;
    }

    AudioEngine* _engine;
    JsonConfigurator* _configurator;
    JsonConfigReturnStatus _status;
    std::string _path;
    Json::Value _config;
    Json::Value _plugin_chain;

    /* Helper function to run "_validate_chains_definition". Makes code more readable */
    JsonConfigReturnStatus _run_validate_chains();
    /* Helper function to run "_make_chain". Makes code more readable */
    JsonConfigReturnStatus _run_make_chain();
};

JsonConfigReturnStatus TestJsonConfigurer::_run_validate_chains()
{
    return _configurator->_validate_chains_definition(_config);
}

JsonConfigReturnStatus TestJsonConfigurer::_run_make_chain()
{
    return _configurator->_make_chain(_plugin_chain);
}

TEST_F(TestJsonConfigurer, TestInstantiation)
{
    EXPECT_TRUE(_engine);
    EXPECT_TRUE(_configurator);
}

TEST_F(TestJsonConfigurer, TestParseFile)
{
    /* Test Successful parsing of file */
    _status = _configurator->_parse_file(_path, _config);
    ASSERT_EQ(_status, JsonConfigReturnStatus::OK);
    ASSERT_TRUE(_config.isMember("stompbox_chains"));
    ASSERT_TRUE(_config.isMember("events"));

    /* Test Unsuccessful parsing of file */
    _status = _configurator->_parse_file("wrong_path", _config);
    ASSERT_EQ(_status, JsonConfigReturnStatus::INVALID_FILE);
}

TEST_F(TestJsonConfigurer, TestCorrectSchema)
{
    _status = _configurator->_parse_file(_path, _config);
    ASSERT_EQ(_status, JsonConfigReturnStatus::OK);
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::OK);
}

TEST_F(TestJsonConfigurer, TestInCorrectChainDef)
{
    /* Tests for case: Plugin chains are not defined in the JSON file */
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);

    /* Define new key "stombox_chains" as null value so it is empty */
    _config["stompbox_chains"] = Json::Value::null;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);
    /* Assign key "stompbox_chains" as empty array */
    _config["stompbox_chains"] = Json::arrayValue;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);

    /* Create Plugin chain array with dummy value but no key "mode" */
    _plugin_chain = Json::arrayValue;
    _plugin_chain[0]["dummy"] = "dummy";
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);

    /* Create key "mode" in "stompbox_chains" array, but defined as empty value */
    _plugin_chain[0]["mode"] = Json::Value::null;
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);
    /* Redefine mode as "monno", instead of "mono" */
    _plugin_chain[0]["mode"] = "monno";
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);

    /* Key "Id" is not defined */
    _plugin_chain[0]["mode"] = "mono";
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_ID);
    /* Define key "ID" but as empty */
    _plugin_chain[0]["id"] = Json::Value::null;
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_ID);
}

TEST_F(TestJsonConfigurer, TestStompboxDef)
{
    _config["stompbox_chains"] = Json::arrayValue;
    _plugin_chain = Json::arrayValue;
    _plugin_chain[0]["mode"] = "mono";
    _plugin_chain[0]["id"] = "chain_name";

    /* Test for case Stompboxes are not defined in the plugin chain*/
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /* Define key "stompboxes" as null value */
    _plugin_chain[0]["stompboxes"] = Json::Value::null;
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);
    /* Defining "stompboxes" as an empty array is valid and should return OK */
    _plugin_chain[0]["stompboxes"] = Json::arrayValue;
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::OK);

    /* Valid stompboxes in plugin chain */
    Json::Value test_stompbox = Json::arrayValue;
    test_stompbox[0]["id"] = "test_id";
    test_stompbox[0]["stompbox_uid"] = "test_uid";
    _plugin_chain[0]["stompboxes"] = test_stompbox;
    _config["stompbox_chains"] = _plugin_chain;
    ASSERT_EQ(_run_validate_chains(), JsonConfigReturnStatus::OK);
}

TEST_F(TestJsonConfigurer, TestLoadChains)
{
    JsonConfigReturnStatus status = _configurator->load_chains(_path);
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);
    ASSERT_EQ(2, _engine->_audio_graph[0]->input_channels());
    ASSERT_EQ(2, _engine->_audio_graph[0]->output_channels());
    ASSERT_EQ(1, _engine->_audio_graph[1]->input_channels());
    ASSERT_EQ(1, _engine->_audio_graph[1]->output_channels());
    auto chain_l = &_engine->_audio_graph[0]->_chain;
    auto chain_r = &_engine->_audio_graph[1]->_chain;
    ASSERT_EQ(chain_l->size(), 3u);
    ASSERT_EQ(chain_r->size(), 3u);
    ASSERT_EQ(1, _engine->_audio_graph[1]->input_channels());

    /* TODO - Is this casting a good idea */
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(0))->name(), "passthrough_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(1))->name(), "gain_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(2))->name(), "equalizer_0_l");

    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(0))->name(), "gain_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(1))->name(), "passthrough_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(2))->name(), "gain_1_r");
}

TEST_F(TestJsonConfigurer, TestMakeChain)
{
    /* Create plugin chain without stompboxes */
    _plugin_chain["mode"] = "mono";
    _plugin_chain["id"] = "chain_without_stomp";
    _plugin_chain["stompboxes"] = Json::arrayValue;
    ASSERT_EQ(_run_make_chain(), JsonConfigReturnStatus::OK);
    /* Similar Plugin chain but with same chain id */
    _plugin_chain["mode"] = "stereo";
    ASSERT_EQ(_run_make_chain(), JsonConfigReturnStatus::INVALID_CHAIN_ID);

    /* Create valid plugin chain with valid stompboxes */
    _plugin_chain["id"] = "chain_with_stomp";
    Json::Value test_stompbox = Json::arrayValue;
    test_stompbox[0]["id"] = "valid_stomp_name";
    test_stompbox[0]["stompbox_uid"] = "sushi.testing.gain";
    _plugin_chain["stompboxes"] = test_stompbox;
    ASSERT_EQ(_run_make_chain(), JsonConfigReturnStatus::OK);

    /* test with stompbox having Invalid UID or existing name */
    _plugin_chain["id"] = "chain_invalid_stompuid";
    test_stompbox[0]["id"] = "invalid_stomp";
    test_stompbox[0]["stompbox_uid"] = "wrong_uid";
    _plugin_chain["stompboxes"] = test_stompbox;
    ASSERT_EQ(_run_make_chain(), JsonConfigReturnStatus::INVALID_STOMPBOX_UID);
    _plugin_chain["id"] = "chain_invalid_stompname";
    test_stompbox[0]["id"] = "valid_stomp_name";
    test_stompbox[0]["stompbox_uid"] = "sushi.testing.gain";
    _plugin_chain["stompboxes"] = test_stompbox;
    ASSERT_EQ(_run_make_chain(), JsonConfigReturnStatus::INVALID_STOMPBOX_NAME);
}