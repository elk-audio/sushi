#include <algorithm>

#include "gtest/gtest.h"
#include "test_utils.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "plugins/biquad_filter.cpp"
#include "library/plugin_manager.h"

#define private public

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
    StompBox* _module_under_test;
};

TEST_F(TestPassthroughPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestPassthroughPlugin, TestInitialization)
{
    StompBoxConfig c;
    c.sample_rate = 44000;
    _module_under_test->init(c);
    ASSERT_TRUE(_module_under_test);
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestPassthroughPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->process(&in_buffer, &out_buffer);
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
        _manager = new StompBoxManager(_module_under_test);
        StompBoxConfig c;
        c.sample_rate = 44000;
        c.controller = static_cast<StompBoxController*>(_manager);
        StompBoxStatus status = _module_under_test->init(c);
        ASSERT_EQ(StompBoxStatus::OK, status);
    }

    void TearDown()
    {
        delete _manager;
    }
    StompBox* _module_under_test;
    StompBoxManager* _manager;
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
    FloatStompBoxParameter* gain_param = static_cast<FloatStompBoxParameter*>(_manager->get_parameter("gain"));
    ASSERT_TRUE(gain_param);
    gain_param->set(6.0f);
    _module_under_test->process(&in_buffer, &out_buffer);
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
        StompBoxConfig c;
        c.sample_rate = 44000;
        _module_under_test = new equalizer_plugin::EqualizerPlugin();
        _module_under_test->init(c);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    StompBox* _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestEqualizerPlugin, TestInitialization)
{
    StompBoxConfig c;
    c.sample_rate = 44000;
    StompBoxStatus status = _module_under_test->init(c);
    ASSERT_EQ(StompBoxStatus::OK, status);
}

// Test silence in -> silence out
TEST_F(TestEqualizerPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    test_utils::fill_sample_buffer(in_buffer, 0.0f);

    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::FREQUENCY, 4000);
    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::GAIN, 2);
    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::Q, 1);
    _module_under_test->process(&in_buffer, &out_buffer);

    test_utils::assert_buffer_value(0.0f, out_buffer);
}


