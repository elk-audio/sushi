
#include <algorithm>
#include <fstream>
#include <iostream>

#include "gtest/gtest.h"
#include "test_utils.h"

#define private public
#define protected public

#include "engine/engine.cpp"
#include "library/mind_allocator.cpp"

constexpr unsigned int SAMPLE_RATE = 44000;
using namespace sushi;
using namespace sushi::engine;

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

TEST_F(TestEngine, TestUidNameMapping)
{
    _module_under_test->create_plugin_chain("left",2);
    auto status = _module_under_test->add_plugin_to_chain("left",
                                                          "sushi.testing.equalizer",
                                                          "equalizer_0_l",
                                                          "",
                                                          PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ObjectId id;
    std::tie(status, id) = _module_under_test->processor_id_from_name("equalizer_0_l");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_TRUE(_module_under_test->_processor_exists(id));
    std::string name;
    std::tie(status, name) = _module_under_test->processor_name_from_id(id);
    ASSERT_TRUE(_module_under_test->_processor_exists(name));
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ("equalizer_0_l", name);

    /* Test with name/id that doesn't match any processors */
    std::tie(status, id) = _module_under_test->processor_id_from_name("not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, name) = _module_under_test->processor_name_from_id(123456);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);

    /* Test Parameter Lookup */
    std::tie(status, id) = _module_under_test->parameter_id_from_name("equalizer_0_l", "q");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    std::tie(status, id) = _module_under_test->parameter_id_from_name("not_found", "gain");
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, id) = _module_under_test->parameter_id_from_name("equalizer_0_l", "not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_PARAMETER, status);
}

TEST_F(TestEngine, TestCreateEmptyPluginChain)
{
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_TRUE(_module_under_test->_processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph.size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->name(),"left");

    /* Invalid name */
    status = _module_under_test->create_plugin_chain("left",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);
    status = _module_under_test->create_plugin_chain("",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);

    /* Invalid number of channels */
    status = _module_under_test->create_plugin_chain("left",3);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
}

TEST_F(TestEngine, TestAddPlugin)
{
    /* Test adding Internal plugin */
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    status = _module_under_test->add_plugin_to_chain("left",
                                                     "sushi.testing.gain",
                                                     "gain_0_r",
                                                     "   ",
                                                     PluginType::INTERNAL);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    char* full_plugin_path = realpath("libvstxsynth.so", NULL);
    status = _module_under_test->add_plugin_to_chain("left",
                                                     "",
                                                     "vst_synth",
                                                     full_plugin_path,
                                                     PluginType::VST2X);
    delete full_plugin_path;
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_TRUE(_module_under_test->_processor_exists("gain_0_r"));
    ASSERT_TRUE(_module_under_test->_processor_exists("vst_synth"));
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain.size(),2u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[0]->name(),"gain_0_r");
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[1]->name(),"vst_synth");

    /* Negative tests */
    status = _module_under_test->add_plugin_to_chain("not_found",
                                                     "sushi.testing.passthrough",
                                                     "dummyname",
                                                     "",
                                                     PluginType::INTERNAL);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);

    status = _module_under_test->add_plugin_to_chain("left",
                                                     "sushi.testing.passthrough",
                                                     "",
                                                     "",
                                                     PluginType::INTERNAL);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_NAME);

    status = _module_under_test->add_plugin_to_chain("left",
                                                     "not_found",
                                                     "dummyname",
                                                     "",
                                                     PluginType::INTERNAL);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_UID);

    status = _module_under_test->add_plugin_to_chain("left",
                                                     "",
                                                     "dummyname",
                                                     "not_found",
                                                     PluginType::VST2X);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_UID);
}

TEST_F(TestEngine, TestSetSamplerate)
{
    auto status = _module_under_test->create_plugin_chain("left",2);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    status = _module_under_test->add_plugin_to_chain("left",
                                                     "sushi.testing.equalizer",
                                                     "eq",
                                                     "",
                                                     PluginType::INTERNAL);
    ASSERT_EQ(status, EngineReturnStatus::OK);
    _module_under_test->set_sample_rate(48000.0f);
    ASSERT_FLOAT_EQ(48000.0f, _module_under_test->sample_rate());
    /* Pretty ugly way of checking that it was actually set, but wth */
    auto eq_plugin = static_cast<equalizer_plugin::EqualizerPlugin*>(_module_under_test->_processors_by_unique_name["eq"].get());
    ASSERT_FLOAT_EQ(48000.0f, eq_plugin->_sample_rate);
}