#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "engine/plugin_chain.cpp"
#include "plugins/passthrough_plugin.h"

using namespace sushi;
using namespace engine;

class DummyProcessor : public Processor
{
public:
    DummyProcessor()
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
    }

    ProcessorReturnCode init(const int /* sample_rate */) override
    {
        return ProcessorReturnCode::OK;
    }

    void process_event(Event /*event*/) override {}
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
    DummyProcessor test_processor;
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

TEST_F(PluginChainTest, test_event_bypass_processing)
{
    ChunkSampleBuffer buffer(2);
    EventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin;
    plugin.init(44100);
    plugin.set_event_output(&event_queue);
    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    Event event = Event::make_note_on_event(0, 0, 0, 0);

    _module_under_test.process_event(event);
    _module_under_test.process_audio(buffer, buffer);
    ASSERT_FALSE(event_queue.empty());
}