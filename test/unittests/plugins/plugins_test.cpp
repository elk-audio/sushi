#include <algorithm>

#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "plugins/biquad_filter.cpp"
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
    // TODO: put again the tests after revising initialization process
    //       (now temporarily in the constructor)
    // StompBoxConfig c;
    // c.sample_rate = 44000;
    // _module_under_test->init(c);
    ASSERT_TRUE(_module_under_test);
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestPassthroughPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0, out_buffer);
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
        // TODO: revise after initialization methods are set
        // StompBoxConfig c;
        // c.sample_rate = 44000;
        // c.controller = static_cast<StompBoxController*>(_manager);
        // StompBoxStatus status = _module_under_test->init(c);
        // ASSERT_EQ(StompBoxStatus::OK, status);
    }

    void TearDown()
    {
    }
    InternalPlugin* _module_under_test;
};

TEST_F(TestGainPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
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
        // TODO: revise after new initialization system
        // StompBoxConfig c;
        // c.sample_rate = 44000;
        // c.controller = _manager;
        // StompBoxStatus status = _module_under_test->init(c);
        // ASSERT_EQ(StompBoxStatus::OK, status);
    }

    void TearDown()
    {
    }
    InternalPlugin* _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
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


