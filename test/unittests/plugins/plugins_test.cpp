#include <algorithm>

#include "gtest/gtest.h"

#define private public

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/internal_plugin.h"
#include "plugins/passthrough_plugin.cpp"
#include "plugins/gain_plugin.cpp"
#include "plugins/lfo_plugin.cpp"
#include "plugins/equalizer_plugin.cpp"
#include "plugins/peak_meter_plugin.cpp"
#include "dsp_library/biquad_filter.cpp"

using namespace sushi;

constexpr float TEST_SAMPLERATE = 48000;

class TestPassthroughPlugin : public ::testing::Test
{
protected:
    TestPassthroughPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new passthrough_plugin::PassthroughPlugin(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
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
    RtSafeRtEventFifo event_queue;
    ASSERT_TRUE(event_queue.empty());
    _module_under_test->set_event_output(&event_queue);
    RtEvent event = RtEvent::make_note_on_event(0, 0, 0, 0, 0);

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
        _module_under_test = new gain_plugin::GainPlugin(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
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
    _module_under_test->_gain_parameter->set(0.525f);
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
        _module_under_test = new equalizer_plugin::EqualizerPlugin(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
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
    _module_under_test->_frequency->set(0.1991991991991992f);
    _module_under_test->_gain->set(0.625f);
    _module_under_test->_q->set(0.1f);

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
        _module_under_test = new peak_meter_plugin::PeakMeterPlugin(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_fifo);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
    peak_meter_plugin::PeakMeterPlugin* _module_under_test;
    RtSafeRtEventFifo _fifo;
};

TEST_F(TestPeakMeterPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Peak Meter", _module_under_test->label());
    ASSERT_EQ("sushi.testing.peakmeter", _module_under_test->name());
}

TEST_F(TestPeakMeterPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    /* Process enough samples to catch some event outputs */
    ASSERT_TRUE(_fifo.empty());
    for (int i = 0; i <= TEST_SAMPLERATE / (peak_meter_plugin::REFRESH_RATE * AUDIO_CHUNK_SIZE) ; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }
    /* check that audio goes through unprocessed */
    test_utils::assert_buffer_value(1.0f, out_buffer);

    RtEvent event;
    ASSERT_TRUE(_fifo.pop(event));
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    EXPECT_EQ(_module_under_test->id(), event.processor_id());
    /*  The value should approach 0 dB eventually, but test that it is reasonably close */
    EXPECT_GT(event.parameter_change_event()->value(), -8.0f);
}

class TestLfoPlugin : public ::testing::Test
{
protected:
    TestLfoPlugin()
    {
    }

    void SetUp()
    {
        _module_under_test = new lfo_plugin::LfoPlugin(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_queue);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
    lfo_plugin::LfoPlugin* _module_under_test;
    RtSafeRtEventFifo _queue;
};

TEST_F(TestLfoPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
    ASSERT_EQ("Lfo", _module_under_test->label());
    ASSERT_EQ("sushi.testing.lfo", _module_under_test->name());
}

TEST_F(TestLfoPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(0);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(0);
    // Calling process should result in a parameter update event.
    _module_under_test->process_audio(in_buffer, out_buffer);
    ASSERT_FALSE(_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_queue.pop(event));
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());

    // Connect a cv output to it
    auto param = _module_under_test->parameter_from_name("out");
    _module_under_test->connect_cv_from_parameter(param->id(), 2);
    // Calling process should now result in a cv event instead.
    _module_under_test->process_audio(in_buffer, out_buffer);
    ASSERT_FALSE(_queue.empty());
    ASSERT_TRUE(_queue.pop(event));
    EXPECT_EQ(RtEventType::CV_EVENT, event.type());
    EXPECT_EQ(2, event.cv_event()->cv_id());
}