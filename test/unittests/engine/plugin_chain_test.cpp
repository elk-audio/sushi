#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "engine/plugin_chain.cpp"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"

using namespace sushi;
using namespace engine;

class DummyProcessor : public Processor
{
public:
    DummyProcessor()
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
    }

    ProcessorReturnCode init(float /* sample_rate */) override
    {
        return ProcessorReturnCode::OK;
    }

    void process_event(RtEvent /*event*/) override {}
    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
    {
        out_buffer = in_buffer;
    }
};

class DummyMonoProcessor : public DummyProcessor
{
public:
    DummyMonoProcessor()
    {
        _max_input_channels = 1;
        _max_output_channels = 1;
        _current_input_channels = _max_input_channels;
        _current_output_channels = _max_output_channels;
    }
};

class PluginChainTest : public ::testing::Test
{
protected:
    PluginChainTest() {}

    PluginChain _module_under_test{2};
};


TEST_F(PluginChainTest, TestChannelManagement)
{
    DummyProcessor test_processor;
    test_processor.set_input_channels(2);
    /* Add the test processor to a mono chain and verify
     * it is configured in mono config */
    PluginChain _module_under_test_mono(1);
    _module_under_test_mono.set_input_channels(1);
    _module_under_test_mono.add(&test_processor);
    EXPECT_EQ(1, test_processor.input_channels());
    EXPECT_EQ(1, test_processor.output_channels());

    /* Put a stereo and then a mono-only plugin on a
     * stereo track */
    gain_plugin::GainPlugin gain_plugin;
    DummyMonoProcessor mono_processor;
    _module_under_test.set_output_channels(1);
    _module_under_test.add(&gain_plugin);
    _module_under_test.add(&mono_processor);

    EXPECT_EQ(2, _module_under_test.input_channels());
    EXPECT_EQ(1, _module_under_test.output_channels());
    EXPECT_EQ(2, gain_plugin.input_channels());
    EXPECT_EQ(1, gain_plugin.output_channels());
    EXPECT_EQ(1, mono_processor.input_channels());
    EXPECT_EQ(1, mono_processor.output_channels());

    /* Set the input to mono and watch the plugins adapt */
    _module_under_test.set_input_channels(1);
    EXPECT_EQ(1, _module_under_test.input_channels());
    EXPECT_EQ(1, gain_plugin.input_channels());
    EXPECT_EQ(1, gain_plugin.output_channels());
}

TEST_F(PluginChainTest, TestAddAndRemove)
{
    DummyProcessor test_processor;
    _module_under_test.add(&test_processor);
    EXPECT_EQ(1u, _module_under_test._chain.size());
    EXPECT_FALSE(_module_under_test.remove(1234567u));
    EXPECT_EQ(1u, _module_under_test._chain.size());
    EXPECT_TRUE(_module_under_test.remove(test_processor.id()));
    EXPECT_TRUE(_module_under_test._chain.empty());
}

TEST_F(PluginChainTest, TestNestedBypass)
{
    DummyProcessor test_processor;
    _module_under_test.add(&test_processor);
    _module_under_test.set_bypassed(true);
    EXPECT_TRUE(test_processor.bypassed());
}

TEST_F(PluginChainTest, TestEmptyChainProcessing)
{
    /* Test that audio goes right through an empty chain unaffected */
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    _module_under_test.set_input_channels(2);
    _module_under_test.set_output_channels(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::assert_buffer_value(1.0f, in_buffer);

    _module_under_test.process_audio(in_buffer, out_buffer);

    test_utils::assert_buffer_value(1.0f, out_buffer);
}

TEST_F(PluginChainTest, TestEventProcessing)
{
    ChunkSampleBuffer buffer(2);
    EventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    passthrough_plugin::PassthroughPlugin plugin;
    plugin.init(44100);
    plugin.set_event_output(&event_queue);
    _module_under_test.set_input_channels(2);
    _module_under_test.set_output_channels(2);
    _module_under_test.set_event_output(&event_queue);
    _module_under_test.add(&plugin);

    RtEvent event = RtEvent::make_note_on_event(0, 0, 0, 0);

    _module_under_test.process_event(event);
    _module_under_test.process_audio(buffer, buffer);
    ASSERT_FALSE(event_queue.empty());
}