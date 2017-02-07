#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "engine/plugin_chain.cpp"

using namespace sushi;
using namespace engine;

class TestProcessor : public Processor
{
public:
    TestProcessor()
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
    }
    void process_event(BaseEvent* /*event*/) override{}
    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
    {
        out_buffer = in_buffer;
    }
};

class PluginChainTest : public ::testing::Test
{
protected:
    PluginChainTest() {}

    PluginChain _module_under_test;
};


TEST_F(PluginChainTest, test_channel_management)
{
    TestProcessor test_processor;
    test_processor.set_input_channels(2);
    /* Add the test processor to a mono chain and verify
     * it is configured in mono config */
    _module_under_test.set_input_channels(1);
    _module_under_test.add(&test_processor);
    ASSERT_EQ(1, test_processor.input_channels());
    ASSERT_EQ(1, _module_under_test.input_channels());

    /* Change the chain to a stereo config and verify
     * the processor is also updated */
    _module_under_test.set_input_channels(2);
    ASSERT_EQ(2, test_processor.input_channels());
    ASSERT_EQ(2, _module_under_test.input_channels());
}


TEST_F(PluginChainTest, test_bypass_processing)
{
    /* Test that audio goes right through an empty chain unaffected */
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::assert_buffer_value(1.0f, in_buffer);

    _module_under_test.process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(1.0f, out_buffer);
}