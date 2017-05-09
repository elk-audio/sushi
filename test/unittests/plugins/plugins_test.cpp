#include <algorithm>

#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "dsp_library/biquad_filter.cpp"
#include "library/internal_plugin.cpp"

using namespace sushi;

/*
 * Tests for passthrough unit gain plugin
 */
class TestPassthroughPlugin : public ::testing::Test
{
protected:
    TestPassthroughPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new passthrough_plugin::PassthroughPlugin();
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    InternalPlugin* _module_under_test;
};

TEST_F(TestPassthroughPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestPassthroughPlugin, TestInitialization)
{
    _module_under_test->init(48000);
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Passthrough", _module_under_test->label());
    ASSERT_EQ("sushi.testing.passthrough", _module_under_test->name());
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestPassthroughPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    EventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    _module_under_test->set_event_output(&event_queue);
    Event event = Event::make_note_on_event(0, 0, 0, 0);

    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0, out_buffer);
    ASSERT_FALSE(event_queue.empty());
}

/*
 * Tests for Gain Plugin
 */
class TestGainPlugin : public ::testing::Test
{
protected:
    TestGainPlugin()
    {
    }

    void SetUp()
    {
        _module_under_test = new gain_plugin::GainPlugin();
        ProcessorReturnCode status = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    InternalPlugin* _module_under_test;
};

TEST_F(TestGainPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Gain", _module_under_test->label());
    ASSERT_EQ("sushi.testing.gain", _module_under_test->name());
}

// Fill a buffer with ones, set gain to 2 and process it
TEST_F(TestGainPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    FloatStompBoxParameter* gain_param = static_cast<FloatStompBoxParameter*>(_module_under_test->get_parameter("gain"));
    ASSERT_TRUE(gain_param);
    gain_param->set(6.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(2.0f, out_buffer, test_utils::DECIBEL_ERROR);
}

/*
* Tests for Equalizer Plugin
*/
class TestEqualizerPlugin : public ::testing::Test
{
protected:
    TestEqualizerPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new equalizer_plugin::EqualizerPlugin();
        ProcessorReturnCode status = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    InternalPlugin* _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Equalizer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.equalizer", _module_under_test->name());
}

// Test silence in -> silence out
TEST_F(TestEqualizerPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 0.0f);

    // Get the registered parameters, check they exist and call set on them.
    FloatStompBoxParameter* freq_param = static_cast<FloatStompBoxParameter*>(_module_under_test->get_parameter("frequency"));
    ASSERT_TRUE(freq_param);

    FloatStompBoxParameter* gain_param = static_cast<FloatStompBoxParameter*>(_module_under_test->get_parameter("gain"));
    ASSERT_TRUE(gain_param);

    FloatStompBoxParameter* q_param = static_cast<FloatStompBoxParameter*>(_module_under_test->get_parameter("q"));
    ASSERT_TRUE(q_param);

    freq_param->set(4000.0f);
    gain_param->set(6.0f);
    q_param->set(1.0f);

    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}


