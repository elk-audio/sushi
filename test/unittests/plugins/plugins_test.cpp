#include <algorithm>

#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "plugins/peak_meter_plugin.cpp"
#include "dsp_library/biquad_filter.cpp"
#include "library/internal_plugin.cpp"

using namespace sushi;


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
    RtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    _module_under_test->set_event_output(&event_queue);
    RtEvent event = RtEvent::make_note_on_event(0, 0, 0, 0);

    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0, out_buffer);
    ASSERT_FALSE(event_queue.empty());
}


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
    gain_plugin::GainPlugin* _module_under_test;
};

TEST_F(TestGainPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Gain", _module_under_test->label());
    ASSERT_EQ("sushi.testing.gain", _module_under_test->name());
}

TEST_F(TestGainPlugin, TestChannelSetup)
{
    _module_under_test->set_input_channels(2);
    ASSERT_EQ(2, _module_under_test->output_channels());
    ASSERT_EQ(2, _module_under_test->input_channels());

    _module_under_test->set_input_channels(1);
    ASSERT_EQ(1, _module_under_test->output_channels());
    ASSERT_EQ(1, _module_under_test->input_channels());
}

// Fill a buffer with ones, set gain to 2 and process it
TEST_F(TestGainPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->set_input_channels(2);
    _module_under_test->set_output_channels(2);
    _module_under_test->_gain_parameter->set(6.0f);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(2.0f, out_buffer, test_utils::DECIBEL_ERROR);
}


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
    equalizer_plugin::EqualizerPlugin* _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Equalizer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.equalizer", _module_under_test->name());
}

TEST_F(TestEqualizerPlugin, TestChannelSetup)
{
    _module_under_test->set_input_channels(2);
    ASSERT_EQ(2, _module_under_test->output_channels());
    ASSERT_EQ(2, _module_under_test->input_channels());

    _module_under_test->set_input_channels(1);
    ASSERT_EQ(1, _module_under_test->output_channels());
    ASSERT_EQ(1, _module_under_test->input_channels());
}

// Test silence in -> silence out
TEST_F(TestEqualizerPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 0.0f);

    // Get the registered parameters, check they exist and call set on them.
    auto freq_param = static_cast<const FloatParameterDescriptor*>(_module_under_test->parameter_from_name("frequency"));
    ASSERT_TRUE(freq_param);

    auto gain_param = static_cast<const FloatParameterDescriptor*>(_module_under_test->parameter_from_name("gain"));
    ASSERT_TRUE(gain_param);

    auto q_param = static_cast<const FloatParameterDescriptor*>(_module_under_test->parameter_from_name("q"));
    ASSERT_TRUE(q_param);

    _module_under_test->set_input_channels(2);
    _module_under_test->_frequency->set(4000.0f);
    _module_under_test->_gain->set(6.0f);
    _module_under_test->_q->set(1.0f);

    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}


class TestPeakMeterPlugin : public ::testing::Test
{
protected:
    TestPeakMeterPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new peak_meter_plugin::PeakMeterPlugin();
        ProcessorReturnCode status = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    peak_meter_plugin::PeakMeterPlugin* _module_under_test;
};

TEST_F(TestPeakMeterPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Peak Meter", _module_under_test->label());
    ASSERT_EQ("sushi.testing.peakmeter", _module_under_test->name());
}