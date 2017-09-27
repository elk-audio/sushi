
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>

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
    _module_under_test->create_plugin_chain("test_chain", 2);

    /* Run tests */
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(4);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(4);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::fill_sample_buffer(out_buffer, 0.5f);

    _module_under_test->process_chunk(&in_buffer, &out_buffer);

    /* Separate the first 2 channels, which should pass through unprocessed
     * and the 2 last, which should be set to 0 */
    auto pair_1 = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);
    auto pair_2 = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 2, 2);

    test_utils::assert_buffer_value(1.0f, pair_1);
    test_utils::assert_buffer_value(0.0f, pair_2);
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

    /* Test invalid name */
    status = _module_under_test->create_plugin_chain("left",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PROCESSOR);
    status = _module_under_test->create_plugin_chain("",1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_NAME);

    /* Test removal */
    status = _module_under_test->delete_plugin_chain("left");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_FALSE(_module_under_test->_processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph.size(),0u);

    /* Test invalid number of channels */
    status = _module_under_test->create_plugin_chain("left",3);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
}

TEST_F(TestEngine, TestAddAndRemovePlugin)
{
    /* Test adding Internal plugin */
    auto status = _module_under_test->create_plugin_chain("left", 2);
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
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain.size(), 2u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[0]->name(),"gain_0_r");
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[1]->name(),"vst_synth");

    /* Test removal of plugin */
    status = _module_under_test->remove_plugin_from_chain("left", "gain_0_r");
    ASSERT_EQ(status, EngineReturnStatus::OK);
    ASSERT_FALSE(_module_under_test->_processor_exists("gain_0_r"));
    ASSERT_EQ(_module_under_test->_audio_graph[0]->_chain[0]->name(),"vst_synth");

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

    status = _module_under_test->remove_plugin_from_chain("left", "unknown_plugin");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_NAME);

    status = _module_under_test->remove_plugin_from_chain("unknown chain", "unknown_plugin");
    ASSERT_EQ(status, EngineReturnStatus::INVALID_PLUGIN_CHAIN);
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
    auto eq_plugin = static_cast<equalizer_plugin::EqualizerPlugin*>(_module_under_test->_processors["eq"].get());
    ASSERT_FLOAT_EQ(48000.0f, eq_plugin->_sample_rate);
}

TEST_F(TestEngine, TestRealtimeConfiguration)
{
    auto faux_rt_thread = [](AudioEngine* e)
    {
        SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
        SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        e->process_chunk(&in_buffer, &out_buffer);
    };
    // Add a chain, then a plugin to it while the engine is running, i.e. do it by asynchronous events instead
    _module_under_test->enable_realtime(true);
    auto rt = std::thread(faux_rt_thread, _module_under_test);
    auto status = _module_under_test->create_plugin_chain("main", 2);
    rt.join();
    ASSERT_EQ(status, EngineReturnStatus::OK);

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->add_plugin_to_chain("main",
                                                     "sushi.testing.gain",
                                                     "gain_0_r",
                                                     "   ",
                                                     PluginType::INTERNAL);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(1u, _module_under_test->_audio_graph[0]->_chain.size());
    auto chain = _module_under_test->_audio_graph[0];
    ObjectId chain_id = chain->id();
    ObjectId processor_id = chain->_chain[0]->id();

    // Remove the plugin and chain as well
    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->remove_plugin_from_chain("main", "gain_0_r");
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph[0]->_chain.size());

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->delete_plugin_chain("main");
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph.size());

    // Assert that they were also deleted from the map of processors
    ASSERT_FALSE(_module_under_test->_processor_exists("main"));
    ASSERT_FALSE(_module_under_test->_processor_exists("gain_0_r"));
    ASSERT_FALSE(_module_under_test->_realtime_processors[chain_id]);
    ASSERT_FALSE(_module_under_test->_realtime_processors[processor_id]);
}