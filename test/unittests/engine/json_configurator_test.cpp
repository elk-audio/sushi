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

class TestJsonConfigurator : public ::testing::Test
{
protected:
    TestJsonConfigurator() {}


    void SetUp()
    {
        _engine = new AudioEngine(SAMPLE_RATE);
        _configurator = new JsonConfigurator(_engine);
        _status = JsonConfigReturnStatus::OK;
        _path = test_utils::get_data_dir_path();
        _path.append("config.json");
        _config = Json::Value::null;
    }

    void TearDown()
    {
        delete _engine;
        delete _configurator;
    }

    /* Helper function to run "_validate_chains_definition". Makes code more readable */
    JsonConfigReturnStatus _validate_chains();

    /* Helper function to run "_validate_midi". Makes code more readable */
    JsonConfigReturnStatus _validate_midi();

    /* Helper function to run "_make_chain". Makes code more readable */
    JsonConfigReturnStatus _make_chain(const Json::Value& plugin_chain);

    AudioEngine* _engine;
    JsonConfigurator* _configurator;
    JsonConfigReturnStatus _status;
    std::string _path;
    Json::Value _config;
};

JsonConfigReturnStatus TestJsonConfigurator::_validate_chains()
{
    return _configurator->_validate_chains_definition(_config);
}

JsonConfigReturnStatus TestJsonConfigurator::_validate_midi()
{
    return _configurator->_validate_midi_definition(_config);
}

JsonConfigReturnStatus TestJsonConfigurator::_make_chain(const Json::Value& plugin_chain)
{
    return _configurator->_make_chain(plugin_chain);
}

TEST_F(TestJsonConfigurator, TestInstantiation)
{
    EXPECT_TRUE(_engine);
    EXPECT_TRUE(_configurator);
}

TEST_F(TestJsonConfigurator, TestLoadChains)
{
    auto status = _configurator->load_chains(_path);
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

TEST_F(TestJsonConfigurator, TestLoadMidi)
{
    auto status = _configurator->load_chains(_path);
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);
    _engine->set_midi_input_ports(1);

    status = _configurator->load_midi(_path);
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);
    ASSERT_EQ(1u, _engine->_midi_dispatcher._kb_routes.size());
    ASSERT_EQ(1u, _engine->_midi_dispatcher._cc_routes.size());
}

TEST_F(TestJsonConfigurator, TestParseFile)
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

TEST_F(TestJsonConfigurator, TestMakeChain)
{
    /* Create plugin chain without stompboxes */
    Json::Value plugin_chain = Json::Value::null;
    plugin_chain["mode"] = "mono";
    plugin_chain["id"] = "chain_without_stomp";
    plugin_chain["stompboxes"] = Json::arrayValue;
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::OK);
    /* Similar Plugin chain but with same chain id */
    plugin_chain["mode"] = "stereo";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_CHAIN_ID);

    /* Create valid plugin chain with valid stompboxes */
    plugin_chain["id"] = "chain_with_stomp";
    auto& test_stompbox = plugin_chain["stompboxes"];
    test_stompbox[0]["id"] = "valid_stomp_name";
    test_stompbox[0]["stompbox_uid"] = "sushi.testing.gain";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::OK);

    /* test with stompbox having Invalid UID or existing name */
    plugin_chain["id"] = "chain_invalid_stompuid";
    test_stompbox[0]["id"] = "invalid_stomp";
    test_stompbox[0]["stompbox_uid"] = "wrong_uid";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_STOMPBOX_UID);
    plugin_chain["id"] = "chain_invalid_stompname";
    test_stompbox[0]["id"] = "valid_stomp_name";
    test_stompbox[0]["stompbox_uid"] = "sushi.testing.gain";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_STOMPBOX_NAME);
}

TEST_F(TestJsonConfigurator, TestValidJsonSchema)
{
    _status = _configurator->_parse_file(_path, _config);
    ASSERT_EQ(_status, JsonConfigReturnStatus::OK);
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::OK);
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::OK);
}

TEST_F(TestJsonConfigurator, TestInvalidPluginChainDef)
{
    /* Tests for case: Plugin chains are not defined in the JSON file */
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);

    /* Define new key "stombox_chains" as null value so it is empty */
    _config["stompbox_chains"] = Json::Value::null;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);
    /* Assign key "stompbox_chains" as empty array */
    _config["stompbox_chains"] = Json::arrayValue;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_FORMAT);

    /* Create Plugin chain array with dummy value but no key "mode" */
    auto& plugin_chain = _config["stompbox_chains"];
    plugin_chain[0]["dummy"] = "dummy";
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);

    /* Create key "mode" in "stompbox_chains" array, but defined as empty value */
    plugin_chain[0]["mode"] = Json::Value::null;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);
    /* Redefine mode as "monno", instead of "mono" */
    plugin_chain[0]["mode"] = "monno";
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_MODE);

    /* Key "Id" is not defined */
    plugin_chain[0]["mode"] = "mono";
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_ID);
    /* Define key "ID" but as empty */
    plugin_chain[0]["id"] = Json::Value::null;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_CHAIN_ID);
}

TEST_F(TestJsonConfigurator, TestStompboxDef)
{
    _config["stompbox_chains"] = Json::arrayValue;
    auto& plugin_chain = _config["stompbox_chains"];
    plugin_chain[0]["mode"] = "mono";
    plugin_chain[0]["id"] = "chain_name";

    /* Test for case Stompboxes are not defined in the plugin chain*/
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /* Define key "stompboxes" as null value */
    plugin_chain[0]["stompboxes"] = Json::Value::null;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);
    /* Defining "stompboxes" as an empty array is valid and should return OK */
    plugin_chain[0]["stompboxes"] = Json::arrayValue;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::OK);

    /* Valid stompboxes in plugin chain */
    Json::Value test_stompbox = Json::arrayValue;
    test_stompbox[0]["id"] = "test_id";
    test_stompbox[0]["stompbox_uid"] = "test_uid";
    plugin_chain[0]["stompboxes"] = test_stompbox;
    ASSERT_EQ(_validate_chains(), JsonConfigReturnStatus::OK);
}

TEST_F(TestJsonConfigurator, TestInValidMidiChainConDef)
{
    Json::Value& midi_def = _config["midi"];
    midi_def["chain_connections"] = Json::arrayValue;
    auto& chain_connections = midi_def["chain_connections"];

    chain_connections[0]["port"] = Json::Value::null;
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_PORT);
    chain_connections[0]["port"] = "wrong_data_type";
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_PORT);
    chain_connections[0]["port"] = 0;

    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_CHAIN_ID);
    chain_connections[0]["chain"] = "left";

    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_CHANNEL);
    chain_connections[0]["channel"] = "wrong_channel";
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_CHANNEL);
    chain_connections[0]["channel"] = 0;

    /* Validate cc_mappings definition. */
    midi_def["cc_mappings"] = Json::arrayValue;
    auto& cc_mapping = midi_def["cc_mappings"];
    cc_mapping[0]["port"] = 0;
    cc_mapping[0]["channel"] = 0;

    /* cc number is not defined. */
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_CC_NUMBER);
    cc_mapping[0]["cc_number"] = 27;

    /* processor name is not defined. */
    cc_mapping[0]["processor"] = Json::Value::null;
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_STOMPBOX_UID);
    cc_mapping[0]["processor"] = "equalizer_0_l";

    /* parameter name is not defined */
    cc_mapping[0]["parameter"] = Json::Value::null;
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_PARAMETER_UID);
    cc_mapping[0]["parameter"] = "gain";

    /* minimum range is not defined */
    cc_mapping[0]["min_range"] = Json::Value::null;
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_RANGE);
    cc_mapping[0]["min_range"] = -24;

    /* maximum range is not defined */
    cc_mapping[0]["max_range"] = Json::Value::null;
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::INVALID_MIDI_RANGE);
    cc_mapping[0]["max_range"] = 24;

    /* everything is defined, schema should be ok here */
    ASSERT_EQ(_validate_midi(), JsonConfigReturnStatus::OK);
}
