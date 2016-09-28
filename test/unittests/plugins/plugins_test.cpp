#include <algorithm>

#include "gtest/gtest.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "plugins/biquad_filter.cpp"

#define private public


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
    AudioProcessorBase* _module_under_test;
};

TEST_F(TestPassthroughPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestPassthroughPlugin, TestInitialization)
{
    AudioProcessorConfig c;
    c.sample_rate = 44000;
    _module_under_test->init(c);
    ASSERT_TRUE(_module_under_test);
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestPassthroughPlugin, TestProcess)
{
    float in_buffer[AUDIO_CHUNK_SIZE];
    float out_buffer[AUDIO_CHUNK_SIZE];
    std::fill(in_buffer, in_buffer + AUDIO_CHUNK_SIZE, 1.0);
    _module_under_test->process(in_buffer, out_buffer);
    for (auto& i : out_buffer)
    {
        ASSERT_FLOAT_EQ(1.0, i);
    }
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
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    AudioProcessorBase* _module_under_test;
};

TEST_F(TestGainPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestGainPlugin, TestInitialization)
{
    AudioProcessorConfig c;
    c.sample_rate = 44000;
    AudioProcessorStatus status = _module_under_test->init(c);
    ASSERT_EQ(AudioProcessorStatus::OK, status);
}

// Fill a buffer with ones, set gain to 2 and process it
TEST_F(TestGainPlugin, TestProcess)
{
    float in_buffer[AUDIO_CHUNK_SIZE];
    float out_buffer[AUDIO_CHUNK_SIZE];
    std::fill(in_buffer, in_buffer + AUDIO_CHUNK_SIZE, 1.0);

    _module_under_test->set_parameter(gain_plugin::gain_parameter_id::GAIN, 2);
    _module_under_test->process(in_buffer, out_buffer);
    for (auto& i : out_buffer)
    {
        ASSERT_FLOAT_EQ(2.0, i);
    }
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
        AudioProcessorConfig c;
        c.sample_rate = 44000;
        _module_under_test = new equalizer_plugin::EqualizerPlugin();
        _module_under_test->init(c);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    AudioProcessorBase* _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

TEST_F(TestEqualizerPlugin, TestInitialization)
{
    AudioProcessorConfig c;
    c.sample_rate = 44000;
    AudioProcessorStatus status = _module_under_test->init(c);
    ASSERT_EQ(AudioProcessorStatus::OK, status);
}

// Test silence in -> silence out
TEST_F(TestEqualizerPlugin, TestProcess)
{
    float in_buffer[AUDIO_CHUNK_SIZE];
    float out_buffer[AUDIO_CHUNK_SIZE];
    std::fill(in_buffer, in_buffer + AUDIO_CHUNK_SIZE, 0);

    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::FREQUENCY, 4000);
    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::GAIN, 2);
    _module_under_test->set_parameter(equalizer_plugin::equalizer_parameter_id::Q, 1);
    _module_under_test->process(in_buffer, out_buffer);

    for (auto& i : out_buffer)
    {
        ASSERT_FLOAT_EQ(0, i);
    }
}


