#include "gtest/gtest.h"

#include "engine/transport.h"

#include "test_utils/test_utils.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "library/processor.cpp"

#include "test_utils/host_control_mockup.h"

using namespace sushi;
using namespace sushi::internal;

namespace sushi::internal
{

class ProcessorAccessor
{
public:
    explicit ProcessorAccessor(Processor& plugin) : _friend(plugin) {}

    [[nodiscard]] bool register_parameter(ParameterDescriptor* parameter)
    {
        return _friend.register_parameter(parameter);
    }

    [[nodiscard]] std::string make_unique_parameter_name(const std::string& name) const
    {
        return _friend._make_unique_parameter_name(name);
    }

    void bypass_process(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
    {
        _friend.bypass_process(in_buffer, out_buffer);
    }

    [[nodiscard]] bool maybe_output_cv_value(ObjectId parameter_id, float value)
    {
        return _friend.maybe_output_cv_value(parameter_id, value);
    }

    [[nodiscard]] bool maybe_output_gate_event(int channel, int note, bool note_on)
    {
        return _friend.maybe_output_gate_event(channel, note, note_on);
    }

private:
    Processor& _friend;
};

}

constexpr float TEST_SAMPLE_RATE = 44100;

constexpr int TEST_BYPASS_TIME_MS = 13;

/* Implement dummies of virtual methods to we can instantiate a test class */
class ProcessorTest : public Processor
{
public:
    explicit ProcessorTest(HostControl host_control) : Processor(host_control)
    {
        _max_input_channels = 2;
        _max_output_channels = 2;
    }

    ~ProcessorTest() override = default;

    void process_audio(const ChunkSampleBuffer& /*in_buffer*/,
                       ChunkSampleBuffer& /*out_buffer*/) override {}

    void process_event(const RtEvent& /*event*/) override {}
};


class TestProcessor : public ::testing::Test
{
protected:
    TestProcessor() = default;

    void SetUp() override
    {
        _module_under_test = std::make_unique<ProcessorTest>(_host_control.make_host_control_mockup());

        _accessor = std::make_unique<sushi::internal::ProcessorAccessor>(*_module_under_test);
    }

    void TearDown() override
    {
    }
    
    HostControlMockup _host_control;
    RtEventFifo<10> _event_queue;

    std::unique_ptr<Processor> _module_under_test;

    std::unique_ptr<sushi::internal::ProcessorAccessor> _accessor;
};

TEST_F(TestProcessor, TestBasicProperties)
{
    /* Set the common properties and verify the changes are applied */
    _module_under_test->set_name(std::string("Processor 1"));
    EXPECT_EQ(_module_under_test->name(), "Processor 1");

    _module_under_test->set_label("processor_1");
    EXPECT_EQ("processor_1", _module_under_test->label());

    _module_under_test->set_enabled(true);
    EXPECT_TRUE(_module_under_test->enabled());
}

TEST_F(TestProcessor, TestParameterHandling)
{
    /* Register a single parameter and verify accessor functions */
    auto p = new FloatParameterDescriptor("param", "Float", "fl", 0, 1, Direction::AUTOMATABLE, nullptr);
    bool success = _accessor->register_parameter(p);
    ASSERT_TRUE(success);

    auto param = _module_under_test->parameter_from_name("not_found");
    EXPECT_FALSE(param);
    param = _module_under_test->parameter_from_name("param");
    EXPECT_TRUE(param);

    ObjectId id = param->id();
    param = _module_under_test->parameter_from_id(id);
    EXPECT_TRUE(param);
    param = _module_under_test->parameter_from_id(1000);
    EXPECT_FALSE(param);

    auto param_list = _module_under_test->all_parameters();
    EXPECT_EQ(1u, param_list.size());
}

TEST_F(TestProcessor, TestDuplicateParameterNames)
{
    bool success = _accessor->register_parameter(new FloatParameterDescriptor("param", "Float", "fl",
                                                                              0, 1, Direction::AUTOMATABLE, nullptr));
    ASSERT_TRUE(success);

    // Test uniqueness by entering an already existing parameter name
    EXPECT_EQ("param_2", _accessor->make_unique_parameter_name("param"));
    EXPECT_EQ("parameter", _accessor->make_unique_parameter_name(""));
}

TEST_F(TestProcessor, TestBypassProcessing)
{
    ChunkSampleBuffer buffer(2);
    ChunkSampleBuffer out_buffer(2);
    ChunkSampleBuffer mono_buffer(1);
    test_utils::fill_sample_buffer(buffer, 1.0f);
    test_utils::fill_sample_buffer(mono_buffer, 2.0f);

    _module_under_test->set_channels(2, 2);
    // Stereo into stereo
    _accessor->bypass_process(buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Mono into stereo
    _module_under_test->set_channels(1, 2);
    _accessor->bypass_process(mono_buffer, out_buffer);
    test_utils::assert_buffer_value(2.0f, out_buffer);

    // No input should clear output
    _module_under_test->set_channels(0, 2);
    _accessor->bypass_process(buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestProcessor, TestCvOutput)
{
    auto p = new FloatParameterDescriptor("param", "Float", "", 0, 1, Direction::AUTOMATABLE, nullptr);
    bool success = _accessor->register_parameter(p);
    ASSERT_TRUE(success);

    _module_under_test->set_event_output(&_event_queue);
    auto param = _module_under_test->parameter_from_name("param");
    ASSERT_TRUE(param);

    // Output parameter update
    auto success_cv_out = _accessor->maybe_output_cv_value(param->id(), 0.5f);
    ASSERT_FALSE(success_cv_out);
    ASSERT_TRUE(_event_queue.empty());

    // Connect parameter to CV output and send update
    auto res = _module_under_test->connect_cv_from_parameter(param->id(), 1);
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    success_cv_out = _accessor->maybe_output_cv_value(param->id(), 0.25f);
    ASSERT_TRUE(success_cv_out);
    ASSERT_FALSE(_event_queue.empty());
    auto cv_event = _event_queue.pop();
    EXPECT_EQ(RtEventType::CV_EVENT, cv_event.type());
    EXPECT_EQ(1, cv_event.cv_event()->cv_id());
    EXPECT_FLOAT_EQ(0.25f, cv_event.cv_event()->value());
}

TEST_F(TestProcessor, TestGateOutput)
{
    _module_under_test->set_event_output(&_event_queue);

    // Output gate update with no connections
    auto success = _accessor->maybe_output_gate_event(5, 10, true);
    ASSERT_FALSE(success);

    // Connect to gate output and send update with another note/channel combo
    auto res = _module_under_test->connect_gate_from_processor(1, 5, 10);
    ASSERT_EQ(ProcessorReturnCode::OK, res);
    success = _accessor->maybe_output_gate_event(4, 9, true);
    ASSERT_FALSE(success);

    // Output gate event
    success = _accessor->maybe_output_gate_event(5, 10, true);
    ASSERT_TRUE(success);
    ASSERT_FALSE(_event_queue.empty());
    auto event = _event_queue.pop();
    EXPECT_EQ(RtEventType::GATE_EVENT, event.type());
    EXPECT_EQ(1, event.gate_event()->gate_no());
    EXPECT_TRUE(event.gate_event()->value());
}

class TestBypassManager : public ::testing::Test
{
protected:
    TestBypassManager() = default;

    BypassManager _module_under_test{false, std::chrono::milliseconds(TEST_BYPASS_TIME_MS)};
};

TEST_F(TestBypassManager, TestOperation)
{
    EXPECT_FALSE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_FALSE(_module_under_test.should_ramp());

    // Set the same condition, nothing should change
    _module_under_test.set_bypass(false, TEST_SAMPLE_RATE);
    EXPECT_FALSE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_FALSE(_module_under_test.should_ramp());

    // Set bypass on
    _module_under_test.set_bypass(true, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.bypassed());
    EXPECT_TRUE(_module_under_test.should_process());
    EXPECT_TRUE(_module_under_test.should_ramp());
}

TEST_F(TestBypassManager, TestSetBypassRampTime)
{
    int expected_chunks = static_cast<int>((TEST_SAMPLE_RATE * TEST_BYPASS_TIME_MS * 0.001) / AUDIO_CHUNK_SIZE);

    // With some sample rate and buffer size combinations this is false.
    if (expected_chunks <= 0)
    {
        // But also in those cases, we want to test with at least one chunk.
        expected_chunks = 1;
    }

    // ... Because chunks_to_rap returns a minimum of 1.
    int to_ramp = _module_under_test.chunks_to_ramp(TEST_SAMPLE_RATE);

    EXPECT_EQ(expected_chunks, to_ramp);
}

TEST_F(TestBypassManager, TestRamping)
{
    int chunks_in_ramp = static_cast<int>((TEST_SAMPLE_RATE * TEST_BYPASS_TIME_MS * 0.001f) / AUDIO_CHUNK_SIZE);

    // With some sample rate and buffer size combinations this is false.
    if (chunks_in_ramp <= 0)
    {
        // But also in those cases, we want to test with at least one chunk.
        chunks_in_ramp = 1;
    }

    ChunkSampleBuffer buffer(2);
    _module_under_test.set_bypass(true, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.should_ramp());

    for (int i = 0; i < chunks_in_ramp; ++i)
    {
        test_utils::fill_sample_buffer(buffer, 1.0f);
        _module_under_test.ramp_output(buffer);
    }

    // We should now have ramped down to 0
    EXPECT_NEAR(0.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1], 1.0e-7);
    EXPECT_NEAR(0.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1], 1.0e-7);
    EXPECT_FLOAT_EQ(1.0f / chunks_in_ramp, buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(1.0f / chunks_in_ramp, buffer.channel(1)[0]);

    EXPECT_FALSE(_module_under_test.should_ramp());

    // Turn it on again (bypass = false)
    _module_under_test.set_bypass(false, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.should_ramp());

    for (int i = 0; i < chunks_in_ramp; ++i)
    {
        test_utils::fill_sample_buffer(buffer, 1.0f);
        _module_under_test.ramp_output(buffer);
    }

    // We should have ramped up to full volume again
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ((chunks_in_ramp - 1.0f) / chunks_in_ramp, buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ((chunks_in_ramp - 1.0f) / chunks_in_ramp, buffer.channel(1)[0]);

    EXPECT_FALSE(_module_under_test.should_ramp());
}

TEST_F(TestBypassManager, TestCrossfade)
{
    int chunks_in_ramp = static_cast<int>((TEST_SAMPLE_RATE * TEST_BYPASS_TIME_MS * 0.001f) / AUDIO_CHUNK_SIZE);
    ChunkSampleBuffer buffer(2);
    ChunkSampleBuffer bypass_buffer(2);
    test_utils::fill_sample_buffer(buffer, 2.0f);
    test_utils::fill_sample_buffer(bypass_buffer, 1);
    _module_under_test.set_bypass(true, TEST_SAMPLE_RATE);
    EXPECT_TRUE(_module_under_test.should_ramp());

    _module_under_test.crossfade_output(bypass_buffer, buffer, 2, 2);

    EXPECT_LE(buffer.channel(1)[AUDIO_CHUNK_SIZE - 1], 2.0f);
    EXPECT_GE(buffer.channel(1)[AUDIO_CHUNK_SIZE - 1], 1.0f);

    for (int i = 0; i < chunks_in_ramp - 1; ++i)
    {
        test_utils::fill_sample_buffer(buffer, 2.0f);
        _module_under_test.crossfade_output(bypass_buffer, buffer, 2, 2);
    }

    // We should now have ramped down to 1 (value of bypass buffer)
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(0)[AUDIO_CHUNK_SIZE - 1]);
    EXPECT_FLOAT_EQ(1.0f, buffer.channel(1)[AUDIO_CHUNK_SIZE - 1]);
}
