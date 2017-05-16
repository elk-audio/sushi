#include "gtest/gtest.h"
#include "test_utils.h"
#include <fstream>
#include <iostream>

#define private public
#define protected public

#include "engine/engine.h"
#include "library/json_configurer.cpp"

using ::testing::internal::posix::GetEnv;


constexpr unsigned int SAMPLE_RATE = 44000;
using namespace sushi;
using namespace sushi::engine;
using namespace sushi::jsonconfigurer;

std::string get_data_dir_path()
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable\n";
    }
    std::string test_config_file(test_data_dir);
    test_config_file.append("/");
    return test_config_file;
};

class TestJsonConfigurer : public ::testing::Test
{
protected:
    TestJsonConfigurer()
    {
    }

    void SetUp()
    {
        _engine = new AudioEngine(SAMPLE_RATE);
        _config = new JsonConfigurer();
        status = JsonConfigReturnStatus::OK;
        path = "";
    }

    void TearDown()
    {
        delete _engine;
        delete _config;
    }


    AudioEngine* _engine;
    JsonConfigurer* _config;
    JsonConfigReturnStatus status;
    std::string path;
};


TEST_F(TestJsonConfigurer, TestInstantiation)
{
    EXPECT_TRUE(_engine);
    EXPECT_TRUE(_config);
}


TEST_F(TestJsonConfigurer, TestInitConfigurer)
{
    /*=====  Test invalid engine and file name  ======*/
    status = _config->init_configurer(_engine,"");
    EXPECT_EQ(status, JsonConfigReturnStatus::INVALID_FILE);
    status = _config->init_configurer(_engine, "dummy");
    EXPECT_EQ(status, JsonConfigReturnStatus::INVALID_FILE);

    /*=====  Test correct instantiation  ======*/
    std::string path = get_data_dir_path();
    path.append("config.json");
    status = _config->init_configurer(_engine, path);
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);
}


TEST_F(TestJsonConfigurer, TestCheckChainDefinition)
{
    Json::Value dummy;
    /*=====  Test if mode is not specified or incorrect  ======*/
    dummy["dummymode"] = "stereo";
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_FILE);

    dummy["mode"];
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_FILE);

    dummy["mode"] = "dummy";
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_CHAIN_MODE);

    /*=====  Test if Chain id is not present or empty  ======*/
    dummy["mode"] = "stereo";
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_CHAIN);

    dummy["id"];
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_CHAIN);

    /*=====  Test if stompbox is incorrectly defined  ======*/
    dummy["id"] = "chainid";
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    dummy["stompboxes"] = Json::Value(Json::arrayValue); 
    status = _config->_check_chain_definition(dummy);
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    dummy["stompboxes"] = "dummy"; 
    status = _config->_check_chain_definition(dummy);     
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Test if stompbox uid or id is not defined in each definition  ======*/
    Json::Value stompboxes;   
    stompboxes[0]["dummy"];
    stompboxes[1]["dummy"];
    dummy["stompboxes"] = stompboxes;
    status = _config->_check_chain_definition(dummy);     
    ASSERT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Test for case: stompbox defined but empty, id undefined  ======*/
    stompboxes[0]["stompbox_uid"];
    EXPECT_TRUE(!stompboxes[0].isMember("stompbox_uid") || stompboxes[0]["stompbox_uid"].empty());
    dummy["stompboxes"] = stompboxes;
    status = _config->_check_chain_definition(dummy);  
    EXPECT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Test for case: stompbox defined, id undefined  ======*/
    stompboxes[0]["stompbox_uid"] = "sushi.testing.passthrough";
    EXPECT_FALSE(!stompboxes[0].isMember("stompbox_uid") || stompboxes[0]["stompbox_uid"].empty());
    dummy["stompboxes"] = stompboxes;
    status = _config->_check_chain_definition(dummy);  
    EXPECT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Test for case: stompbox defined, id defined but empty  ======*/
    stompboxes[0]["id"];
    EXPECT_FALSE(!stompboxes[0].isMember("stompbox_uid") || stompboxes[0]["stompbox_uid"].empty());
    EXPECT_TRUE(!stompboxes[0].isMember("id") || stompboxes[0]["id"].empty());
    dummy["stompboxes"] = stompboxes;
    status = _config->_check_chain_definition(dummy);  
    EXPECT_EQ(status, JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Test for correct instantiation  ======*/
    stompboxes[0]["id"] = "passthrough_0_l";
    stompboxes[1]["stompbox_uid"] = "sushi.testing.gain";
    stompboxes[1]["id"] = "gain_0_r";
    EXPECT_FALSE(!stompboxes[0].isMember("stompbox_uid") || stompboxes[0]["stompbox_uid"].empty());
    EXPECT_FALSE(!stompboxes[0].isMember("id") || stompboxes[0]["id"].empty());
    dummy["stompboxes"] = stompboxes;
    status = _config->_check_chain_definition(dummy);  
    EXPECT_EQ(status, JsonConfigReturnStatus::OK);
}


TEST_F(TestJsonConfigurer, TestCheckStompboxChainsDefinition)
{
      /*=====  Case : stompbox chains is not defined  ======*/
    Json::Value dummy;
    dummy["dummystomp"] = "dummy value";
    _config->_config = dummy;
    status = _config->_check_stompbox_chains_definition();
    ASSERT_EQ(status,JsonConfigReturnStatus::INVALID_STOMPBOX_FORMAT);

    /*=====  Case : stompbox chains is not array  ======*/
    dummy["stompbox_chains"];
    EXPECT_TRUE(!dummy["stompbox_chains"].isArray() || dummy["stompbox_chains"].empty());
    _config->_config = dummy;
    status = _config->_check_stompbox_chains_definition();        
    ASSERT_EQ(status,JsonConfigReturnStatus::INVALID_CHAIN_SIZE); 

    /*=====  Case : stompbox chains is empty array  ======*/
    dummy["stompbox_chains"] = Json::Value(Json::arrayValue); 
    EXPECT_TRUE(!dummy["stompbox_chains"].isArray() || dummy["stompbox_chains"].empty());
    _config->_config = dummy;
    status = _config->_check_stompbox_chains_definition();        
    ASSERT_EQ(status,JsonConfigReturnStatus::INVALID_CHAIN_SIZE);     

    /*=====  Case : stompbox chains is OK  ======*/
    Json::Value dummyarray;
    dummyarray[0]["dummy"] = "dummy";
    dummyarray[1]["dummy"] = "dummy";
    dummy["stompbox_chains"] = dummyarray;
    EXPECT_FALSE(!dummy["stompbox_chains"].isArray() || dummy["stompbox_chains"].empty());
    _config->_config = dummy;
    status = _config->_check_stompbox_chains_definition();        
    ASSERT_EQ(status,JsonConfigReturnStatus::OK);     
}

TEST_F(TestJsonConfigurer, TestInitChains)
{
      /*=====  Test whether the init_chains_from_jsonconfig" method works  ======*/
    std::string path = get_data_dir_path();
    path.append("config.json");
    status = _config->init_configurer(_engine, path);
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);
    status = _config->init_chains_from_jsonconfig();
    ASSERT_EQ(status, JsonConfigReturnStatus::OK);

    EXPECT_EQ(2, _engine->_audio_graph[0]->input_channels());
    EXPECT_EQ(2, _engine->_audio_graph[0]->output_channels());
    EXPECT_EQ(1, _engine->_audio_graph[1]->input_channels());
    EXPECT_EQ(1, _engine->_audio_graph[1]->output_channels());

    auto chain_l = &_engine->_audio_graph[0]->_chain;
    auto chain_r = &_engine->_audio_graph[1]->_chain;

    ASSERT_EQ(chain_l->size(), 3u);

    ASSERT_EQ(chain_r->size(), 3u);
    EXPECT_EQ(1, _engine->_audio_graph[1]->input_channels());

    /* TODO - Is this casting a good idea */
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(0))->name(), "passthrough_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(1))->name(), "gain_0_l");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_l->at(2))->name(), "equalizer_0_l");

    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(0))->name(), "gain_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(1))->name(), "passthrough_0_r");
    ASSERT_EQ(static_cast<InternalPlugin*>(chain_r->at(2))->name(), "gain_1_r");
}