#include <fstream>
#include "gtest/gtest.h"

#define private public
#define protected public

#include "engine/engine.h"
#include "engine/midi_dispatcher.h"
#include "engine/json_configurator.cpp"
#include "test_utils.h"

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
        _midi_dispatcher = new MidiDispatcher(_engine);
        _module_under_test = new JsonConfigurator(_engine, _midi_dispatcher);
        _path = test_utils::get_data_dir_path();
        _path.append("config.json");
    }

    void TearDown()
    {
        delete _module_under_test;
        delete _midi_dispatcher;
        delete _engine;
    }

    /* Helper functions */
    JsonConfigReturnStatus _make_chain(const rapidjson::Value& plugin_chain);

    AudioEngine* _engine;
    MidiDispatcher* _midi_dispatcher;
    JsonConfigurator* _module_under_test;
    std::string _path;
};

JsonConfigReturnStatus TestJsonConfigurator::_make_chain(const rapidjson::Value& plugin_chain)
{
    return _module_under_test->_make_chain(plugin_chain);
}

TEST_F(TestJsonConfigurator, TestInstantiation)
{
    EXPECT_TRUE(_engine);
    EXPECT_TRUE(_module_under_test);
}

TEST_F(TestJsonConfigurator, TestLoadHostConfig)
{
    auto status = _module_under_test->load_host_config(_path);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_FLOAT_EQ(48000.0f, _engine->sample_rate());
}

TEST_F(TestJsonConfigurator, TestLoadChains)
{
    auto status = _module_under_test->load_chains(_path);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_EQ(2, _engine->_audio_graph[0]->input_channels());
    ASSERT_EQ(2, _engine->_audio_graph[0]->output_channels());
    ASSERT_EQ(1, _engine->_audio_graph[1]->input_channels());
    ASSERT_EQ(1, _engine->_audio_graph[1]->output_channels());
    auto chain_l = &_engine->_audio_graph[0]->_chain;
    auto chain_r = &_engine->_audio_graph[1]->_chain;
    ASSERT_EQ(3u, chain_l->size());
    ASSERT_EQ(3u, chain_r->size());
    ASSERT_EQ(1, _engine->_audio_graph[1]->input_channels());

    /* TODO - Is this casting a good idea */
    ASSERT_EQ("passthrough_0_l", static_cast<InternalPlugin*>(chain_l->at(0))->name());
    ASSERT_EQ("gain_0_l", static_cast<InternalPlugin*>(chain_l->at(1))->name());
    ASSERT_EQ("equalizer_0_l", static_cast<InternalPlugin*>(chain_l->at(2))->name());

    ASSERT_EQ("gain_0_r", static_cast<InternalPlugin*>(chain_r->at(0))->name());
    ASSERT_EQ("passthrough_0_r", static_cast<InternalPlugin*>(chain_r->at(1))->name());
    ASSERT_EQ("gain_1_r", static_cast<InternalPlugin*>(chain_r->at(2))->name());
}

TEST_F(TestJsonConfigurator, TestLoadMidi)
{
    auto status = _module_under_test->load_chains(_path);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    _midi_dispatcher->set_midi_input_ports(1);

    status = _module_under_test->load_midi(_path);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_EQ(1u, _midi_dispatcher->_kb_routes_in.size());
    ASSERT_EQ(1u, _midi_dispatcher->_cc_routes.size());
    ASSERT_EQ(1u, _midi_dispatcher->_raw_routes_in.size());
}

TEST_F(TestJsonConfigurator, TestParseFile)
{
    /* Test Successful parsing of file */
    rapidjson::Document config;
    auto status = _module_under_test->_parse_file(_path, config, JsonSection::HOST_CONFIG);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_TRUE(config.HasMember("plugin_chains"));
    ASSERT_TRUE(config.HasMember("midi"));
    ASSERT_TRUE(config.HasMember("events"));

    /* Test Unsuccessful parsing of file */
    status = _module_under_test->_parse_file("wrong_path", config, JsonSection::HOST_CONFIG);
    ASSERT_EQ(JsonConfigReturnStatus::INVALID_FILE, status);
}

TEST_F(TestJsonConfigurator, TestMakeChain)
{
    /* Create plugin chain without stompboxes */
    rapidjson::Document test_cfg;
    rapidjson::Value plugin_chain(rapidjson::kObjectType);
    rapidjson::Value mode("mono");
    rapidjson::Value name("chain_without_plugins");
    rapidjson::Value plugins(rapidjson::kArrayType);
    plugin_chain.AddMember("mode", mode, test_cfg.GetAllocator());
    plugin_chain.AddMember("name", name, test_cfg.GetAllocator());
    plugin_chain.AddMember("plugins", plugins, test_cfg.GetAllocator());
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::OK);

    /* Similar Plugin chain but with same chain id */
    plugin_chain["mode"] = "stereo";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_CHAIN_NAME);

    /* Create valid plugin chain with valid plugin */
    plugin_chain["name"] = "chain_with_internal_plugin";
    rapidjson::Value test_plugin(rapidjson::kObjectType);
    rapidjson::Value uid("sushi.testing.gain");
    rapidjson::Value path("empty_path");
    rapidjson::Value type("internal");
    rapidjson::Value plugin_name("internal_plugin");
    test_plugin.AddMember("uid", uid, test_cfg.GetAllocator());
    test_plugin.AddMember("path", path, test_cfg.GetAllocator());
    test_plugin.AddMember("type", type, test_cfg.GetAllocator());
    test_plugin.AddMember("name", plugin_name, test_cfg.GetAllocator());
    plugin_chain["plugins"].PushBack(test_plugin, test_cfg.GetAllocator());
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::OK);

    /* Add vst plugin  */
    rapidjson::Value& plugin = plugin_chain["plugins"][0];
    const char* full_plugin_path = realpath("libagain.so", NULL);
    plugin_chain["name"] = "chain_with_vst2_plugin";
    plugin["name"] = "vst2_plugin";
    plugin["path"].SetString(full_plugin_path, test_cfg.GetAllocator());
    plugin["type"] = "vst2x";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::OK);
    delete full_plugin_path;

    plugin_chain["name"] = "chain_invalid_internal";
    plugin["name"] = "invalid_internal_plugin";
    plugin["uid"] = "wrong_uid";
    plugin["type"] = "internal";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_PLUGIN_PATH);

    plugin_chain["name"] = "chain_invalid_stompname";
    plugin["name"] = "internal_plugin";
    plugin["uid"] = "sushi.testing.gain";
    plugin["type"] = "internal";
    ASSERT_EQ(_make_chain(plugin_chain), JsonConfigReturnStatus::INVALID_PLUGIN_NAME);
}

TEST_F(TestJsonConfigurator, TestValidJsonSchema)
{
    std::ifstream config_file(_path);
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
    rapidjson::Document test_cfg;
    test_cfg.Parse(config_file_contents.c_str());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::HOST_CONFIG));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::CHAINS));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::MIDI));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::EVENTS));
}

TEST_F(TestJsonConfigurator, TestHostConfigSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();
    /* No definition of host_config */
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));

    /* no definition of samplerate */
    rapidjson::Value host_config(rapidjson::kObjectType);
    test_cfg.AddMember("host_config", host_config, test_cfg.GetAllocator());
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));

    /* invalid type */
    rapidjson::Value samplerate(rapidjson::kObjectType);
    test_cfg["host_config"].AddMember("samplerate", samplerate, test_cfg.GetAllocator());
    test_cfg["host_config"]["samplerate"] = "44100";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));
}

TEST_F(TestJsonConfigurator, TestPluginChainSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();

    rapidjson::Value plugin_chains(rapidjson::kArrayType);
    test_cfg.AddMember("plugin_chains", plugin_chains, test_cfg.GetAllocator());

    /* Plugin chain without plugin list defined is not ok, empty list defined is ok */
    rapidjson::Value example_plugin_chain(rapidjson::kObjectType);
    rapidjson::Value mode("mono");
    rapidjson::Value name("plugin_chain_name");
    rapidjson::Value plugins(rapidjson::kArrayType);
    example_plugin_chain.AddMember("mode", mode, test_cfg.GetAllocator());
    example_plugin_chain.AddMember("name", name, test_cfg.GetAllocator());
    test_cfg["plugin_chains"].PushBack(example_plugin_chain, test_cfg.GetAllocator());
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    test_cfg["plugin_chains"][0].AddMember("plugins", plugins, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));

    /* incorrect mode */
    test_cfg["plugin_chains"][0]["mode"] = "invalid_mode";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    test_cfg["plugin_chains"][0]["mode"] = "stereo";
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
}

TEST_F(TestJsonConfigurator, TestPluginSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();
    rapidjson::Value plugin_chains(rapidjson::kArrayType);
    test_cfg.AddMember("plugin_chains", plugin_chains, test_cfg.GetAllocator());

    rapidjson::Value example_plugin_chain(rapidjson::kObjectType);
    rapidjson::Value chain_name("chain_name");
    rapidjson::Value mode("mono");
    rapidjson::Value plugins(rapidjson::kArrayType);
    example_plugin_chain.AddMember("name", chain_name, test_cfg.GetAllocator());
    example_plugin_chain.AddMember("mode", mode, test_cfg.GetAllocator());
    example_plugin_chain.AddMember("plugins", plugins, test_cfg.GetAllocator());
    test_cfg["plugin_chains"].PushBack(example_plugin_chain, test_cfg.GetAllocator());

    rapidjson::Value example_plugin(rapidjson::kObjectType);
    rapidjson::Value plugin_name("plugin_name");
    rapidjson::Value path("plugin_path");
    rapidjson::Value uid("plugin_name");
    rapidjson::Value type("internal");
    example_plugin.AddMember("name", plugin_name, test_cfg.GetAllocator());
    example_plugin.AddMember("type", type, test_cfg.GetAllocator());
    test_cfg["plugin_chains"][0]["plugins"].PushBack(example_plugin, test_cfg.GetAllocator());
    rapidjson::Value& plugin = test_cfg["plugin_chains"][0]["plugins"][0];

    /* type = internal; requires uid */
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    plugin.AddMember("uid", uid, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    plugin["type"] = "vst3x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));

    /* type = vst2x; requires path */
    plugin["type"] = "vst2x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    plugin.AddMember("path", path, test_cfg.GetAllocator());
    plugin.RemoveMember("uid");
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
    plugin["type"] = "vst3x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));

    /* type = vst3x; requires uid & path */
    rapidjson::Value vst3_uid("vst3_uid");
    plugin.AddMember("uid", vst3_uid, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::CHAINS));
}

TEST_F(TestJsonConfigurator, TestMidiSchema)
{
    rapidjson::Document test_cfg;
    _module_under_test->_parse_file(_path, test_cfg, JsonSection::MIDI);
    rapidjson::Value& chain_connections = test_cfg["midi"]["chain_connections"][0];
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::MIDI));

    chain_connections["channel"] = "invalid";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::MIDI));
    chain_connections["channel"] = 16;
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::MIDI));
}