#include <algorithm>
#include <memory>

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
#include "plugins/wav_writer_plugin.cpp"
#include "plugins/mono_summing_plugin.cpp"
#include "plugins/sample_delay_plugin.cpp"
#include "plugins/stereo_mixer_plugin.cpp"
#include "dsp_library/biquad_filter.cpp"

using namespace sushi;

constexpr float TEST_SAMPLERATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;
static const std::string WRITE_FILE = "write_test";

class TestPassthroughPlugin : public ::testing::Test
{
protected:
    TestPassthroughPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = std::make_unique<passthrough_plugin::PassthroughPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
    }

    HostControlMockup _host_control;
    std::unique_ptr<InternalPlugin> _module_under_test;
};

TEST_F(TestPassthroughPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
}

TEST_F(TestPassthroughPlugin, TestInitialization)
{
    _module_under_test->init(48000);
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Passthrough", _module_under_test->label());
    ASSERT_EQ("sushi.testing.passthrough", _module_under_test->name());
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestPassthroughPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
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
        _module_under_test = std::make_unique<gain_plugin::GainPlugin>(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    HostControlMockup _host_control;
    std::unique_ptr<gain_plugin::GainPlugin> _module_under_test;
};

TEST_F(TestGainPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Gain", _module_under_test->label());
    ASSERT_EQ("sushi.testing.gain", _module_under_test->name());
    ASSERT_EQ(gain_plugin::GainPlugin::static_uid(), _module_under_test->uid());
}

TEST_F(TestGainPlugin, TestChannelSetup)
{
    ASSERT_EQ(2, _module_under_test->output_channels());
    ASSERT_EQ(2, _module_under_test->input_channels());

    _module_under_test->set_input_channels(1);
    _module_under_test->set_output_channels(1);
    ASSERT_EQ(1, _module_under_test->output_channels());
    ASSERT_EQ(1, _module_under_test->input_channels());
}

// Fill a buffer with ones, set gain to 2 and process it
TEST_F(TestGainPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    _module_under_test->_gain_parameter->set(0.875f);
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
        _module_under_test = std::make_unique<equalizer_plugin::EqualizerPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }

    HostControlMockup _host_control;
    std::unique_ptr<equalizer_plugin::EqualizerPlugin> _module_under_test;
};

TEST_F(TestEqualizerPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Equalizer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.equalizer", _module_under_test->name());
}

TEST_F(TestEqualizerPlugin, TestChannelSetup)
{
    ASSERT_EQ(2, _module_under_test->output_channels());
    ASSERT_EQ(2, _module_under_test->input_channels());

    _module_under_test->set_input_channels(1);
    _module_under_test->set_output_channels(1);
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
        _module_under_test = std::make_unique<peak_meter_plugin::PeakMeterPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_event_output(&_fifo);
    }

    HostControlMockup _host_control;
    std::unique_ptr<peak_meter_plugin::PeakMeterPlugin> _module_under_test;
    RtSafeRtEventFifo _fifo;
};

TEST_F(TestPeakMeterPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Peak Meter", _module_under_test->label());
    ASSERT_EQ("sushi.testing.peakmeter", _module_under_test->name());
}

TEST_F(TestPeakMeterPlugin, TestProcess)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    test_utils::fill_sample_buffer(in_buffer, 0.5f);

    /* Process enough samples to catch some event outputs */
    int no_of_process_calls = TEST_SAMPLERATE / (peak_meter_plugin::DEFAULT_REFRESH_RATE * AUDIO_CHUNK_SIZE);
    ASSERT_TRUE(_fifo.empty());
    for (int i = 0; i <= no_of_process_calls ; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }
    /* check that audio goes through unprocessed */
    test_utils::assert_buffer_value(0.5f, out_buffer);

    RtEvent event;
    ASSERT_TRUE(_fifo.pop(event));
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    EXPECT_EQ(_module_under_test->id(), event.processor_id());
    /*  The rms and dB calculations are tested separately, just test that it is a reasonable value */
    EXPECT_GT(event.parameter_change_event()->value(), 0.5f);

    /* Set the rate parameter to minimum */
    auto rate_id = _module_under_test->parameter_from_name("update_rate")->id();
    _module_under_test->process_event(RtEvent::make_parameter_change_event(_module_under_test->id(),
                                                                           0, rate_id, 0.0f));
    while (_fifo.pop(event)) {}

    ASSERT_TRUE(_fifo.empty());
    for (int i = 0; i <= no_of_process_calls * 5 ; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }
    ASSERT_TRUE(_fifo.empty());
}

TEST_F(TestPeakMeterPlugin, TestClipDetection)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    auto first_channel = ChunkSampleBuffer::create_non_owning_buffer(in_buffer, 0, 1);
    test_utils::fill_sample_buffer(in_buffer, 0.5f);
    test_utils::fill_sample_buffer(first_channel, 1.5f);

    auto clip_ch_0_id = _module_under_test->parameter_from_name("clip_0")->id();
    auto clip_ch_1_id = _module_under_test->parameter_from_name("clip_1")->id();

    EXPECT_FLOAT_EQ(0.0f,_module_under_test->parameter_value(clip_ch_0_id).second);
    EXPECT_FLOAT_EQ(0.0f,_module_under_test->parameter_value(clip_ch_1_id).second);

    /* Run once and check that the parameter value has changed for the left channel*/
    _module_under_test->process_audio(in_buffer, out_buffer);
    EXPECT_FLOAT_EQ(1.0f,_module_under_test->parameter_value(clip_ch_0_id).second);
    EXPECT_FLOAT_EQ(0.0f,_module_under_test->parameter_value(clip_ch_1_id).second);

    /* Lower volume and run until the hold time has passed */
    test_utils::fill_sample_buffer(in_buffer, 0.5f);
    for (int i = 0; i <= TEST_SAMPLERATE * 6 / AUDIO_CHUNK_SIZE ; ++i)
    {
        _module_under_test->process_audio(in_buffer, out_buffer);
    }

    EXPECT_FLOAT_EQ(0.0f,_module_under_test->parameter_value(clip_ch_0_id).second);
    EXPECT_FLOAT_EQ(0.0f,_module_under_test->parameter_value(clip_ch_1_id).second);

    /* Pop the first event and verify it was a clip parameter change */
    RtEvent event;
    ASSERT_TRUE(_fifo.pop(event));
    EXPECT_EQ(RtEventType::FLOAT_PARAMETER_CHANGE, event.type());
    EXPECT_EQ(clip_ch_0_id, event.parameter_change_event()->param_id());

    /* Test with linked channels */
    test_utils::fill_sample_buffer(first_channel, 1.5f);
    event = RtEvent::make_parameter_change_event(0,0,_module_under_test->parameter_from_name("link_channels")->id(), 1.0f);
    _module_under_test->process_event(event);
    /* Run once and check that the parameter value has changed for both channels */
    _module_under_test->process_audio(in_buffer, out_buffer);
    EXPECT_FLOAT_EQ(1.0f,_module_under_test->parameter_value(clip_ch_0_id).second);
    EXPECT_FLOAT_EQ(1.0f,_module_under_test->parameter_value(clip_ch_1_id).second);
}

TEST(TestPeakMeterPluginInternal, TestTodBConversion)
{
    EXPECT_FLOAT_EQ(0.0f, peak_meter_plugin::to_normalised_dB(0.0f));         // minimum
    EXPECT_NEAR(0.5f, peak_meter_plugin::to_normalised_dB(0.003981), 0.0001); // -48 dB
    EXPECT_NEAR(0.8333f, peak_meter_plugin::to_normalised_dB(1.0f), 0.0001);  //  0 dB
    EXPECT_FLOAT_EQ(1.0f, peak_meter_plugin::to_normalised_dB(15.9f));        // +24 dB
    EXPECT_FLOAT_EQ(1.0f, peak_meter_plugin::to_normalised_dB(251.2f));       // +48 dB (clamped)
}

class TestLfoPlugin : public ::testing::Test
{
protected:
    TestLfoPlugin()
    {
    }

    void SetUp()
    {
        _module_under_test = std::make_unique<lfo_plugin::LfoPlugin>(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_queue);
        _module_under_test->set_enabled(true);
    }

    HostControlMockup _host_control;
    std::unique_ptr<lfo_plugin::LfoPlugin> _module_under_test;
    RtSafeRtEventFifo _queue;
};

TEST_F(TestLfoPlugin, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test.get());
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

class TestWavWriterPlugin : public ::testing::Test
{
protected:
    TestWavWriterPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = std::make_unique<wav_writer_plugin::WavWriterPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        auto status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_event_output(&_fifo);
        _module_under_test->set_enabled(true);
        _module_under_test->set_input_channels(wav_writer_plugin::N_AUDIO_CHANNELS);
        _module_under_test->set_output_channels(wav_writer_plugin::N_AUDIO_CHANNELS);
    }

    HostControlMockup _host_control;
    std::unique_ptr<wav_writer_plugin::WavWriterPlugin> _module_under_test;
    RtEventFifo<10> _fifo;
};

TEST_F(TestWavWriterPlugin, TestInitialization)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Wav writer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.wav_writer", _module_under_test->name());
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestWavWriterPlugin, TestProcess)
{
    ObjectId record_param_id = _module_under_test->parameter_from_name("recording")->id();
    ObjectId file_property_id = _module_under_test->parameter_from_name("destination_file")->id();

    // Set up buffers and events
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(wav_writer_plugin::N_AUDIO_CHANNELS);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(wav_writer_plugin::N_AUDIO_CHANNELS);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    std::string path = "./";
    path.append(WRITE_FILE);
    RtEvent start_recording_event = RtEvent::make_parameter_change_event(0, 0, record_param_id, 1.0f);
    RtEvent stop_recording_event = RtEvent::make_parameter_change_event(0, 0, record_param_id, 0.0f);

    // Test setting path property
    _module_under_test->set_property_value(file_property_id, path);

    // Test start recording and open file
    _module_under_test->process_event(start_recording_event);
    ASSERT_TRUE(_module_under_test->_recording_parameter->domain_value());
    ASSERT_EQ(wav_writer_plugin::WavWriterStatus::SUCCESS, _module_under_test->_start_recording());

    // Test processing
    _module_under_test->_recording_parameter->set_values(true, true);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(1.0f, in_buffer);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Test Writing.
    _module_under_test->_recording_parameter->set_values(false, false); // set recording to false, to immediately write
    ASSERT_EQ(_module_under_test->input_channels() * AUDIO_CHUNK_SIZE, _module_under_test->_write_to_file());

    // Test end recording and close file
    _module_under_test->process_event(stop_recording_event);
    ASSERT_FALSE(_module_under_test->_recording_parameter->domain_value());
    ASSERT_EQ(wav_writer_plugin::WavWriterStatus::SUCCESS, _module_under_test->_stop_recording());

    // Verify written samples
    path.append(".wav");
    SF_INFO soundfile_info;
    memset(&soundfile_info, 0, sizeof(SF_INFO));
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &soundfile_info);
    if (sf_error(file))
    {
        FAIL() << "While opening file " << path.c_str() << " " << sf_strerror(file) << std::endl;
    }
    int number_of_samples = AUDIO_CHUNK_SIZE * _module_under_test->input_channels();
    float written_data[number_of_samples];
    ASSERT_EQ(AUDIO_CHUNK_SIZE, sf_readf_float(file, written_data, AUDIO_CHUNK_SIZE));
    for (int sample = 0; sample < number_of_samples; ++sample)
    {
        ASSERT_FLOAT_EQ(1.0f, written_data[sample]);
    }
    sf_close(file);
    remove(path.c_str());
}

class TestMonoSummingPlugin : public ::testing::Test
{
protected:
    TestMonoSummingPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = std::make_unique<mono_summing_plugin::MonoSummingPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        auto status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_enabled(true);
        _module_under_test->set_event_output(&_fifo);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);

    }

    HostControlMockup _host_control;
    std::unique_ptr<mono_summing_plugin::MonoSummingPlugin> _module_under_test;
    RtEventFifo<10> _fifo;
};

TEST_F(TestMonoSummingPlugin, TestInitialization)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Mono summing", _module_under_test->label());
    ASSERT_EQ("sushi.testing.mono_summing", _module_under_test->name());
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestMonoSummingPlugin, TestProcess)
{
    // Set up buffers and events
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    for (int sample = 0; sample < AUDIO_CHUNK_SIZE; ++sample)
    {
        in_buffer.channel(0)[sample] = 1.0f;
    }
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);

    _module_under_test->process_audio(in_buffer, out_buffer);
    for (int sample = 0; sample < AUDIO_CHUNK_SIZE; ++sample)
    {
        ASSERT_FLOAT_EQ(1.0f, in_buffer.channel(0)[sample]);
        ASSERT_FLOAT_EQ(0.0f, in_buffer.channel(1)[sample]);
    }
    test_utils::assert_buffer_value(1.0f, out_buffer);
}

class TestSampleDelayPlugin : public ::testing::Test
{
protected:
    TestSampleDelayPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = std::make_unique<sample_delay_plugin::SampleDelayPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        auto status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_enabled(true);
        _module_under_test->set_event_output(&_fifo);
    }

    HostControlMockup _host_control;
    std::unique_ptr<sample_delay_plugin::SampleDelayPlugin> _module_under_test;
    RtSafeRtEventFifo _fifo;
};

TEST_F(TestSampleDelayPlugin, TestInitialization)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Sample delay", _module_under_test->label());
    ASSERT_EQ("sushi.testing.sample_delay", _module_under_test->name());
}

// Fill a buffer with ones and test that they are passed through unchanged
TEST_F(TestSampleDelayPlugin, TestProcess)
{
    // Set up data
    int n_audio_channels = TEST_CHANNEL_COUNT;
    std::vector<int> delay_times = {0, 1, 5, 20, 62, 15, 2};
    SampleBuffer<AUDIO_CHUNK_SIZE> zero_buffer(n_audio_channels);
    SampleBuffer<AUDIO_CHUNK_SIZE> result_buffer(n_audio_channels);
    SampleBuffer<AUDIO_CHUNK_SIZE> impulse_buffer(n_audio_channels);
    for (int channel = 0; channel < n_audio_channels; channel++)
    {
        impulse_buffer.channel(channel)[0] = 1.0;
    }

    // Test processing
    for (auto delay_time : delay_times)
    {
        // Parameter change event
        auto delay_time_event = RtEvent::make_parameter_change_event(0, 0, 0, static_cast<float>(delay_time) / 48000.0f);
        _module_under_test->process_event(delay_time_event);

        // Process audio
        _module_under_test->process_audio(zero_buffer, result_buffer);
        _module_under_test->process_audio(impulse_buffer, result_buffer);

        // Check the impulse has been delayed the correct number of samples
        for (int sample_idx = 0; sample_idx < AUDIO_CHUNK_SIZE; sample_idx++)
        {
            for (int channel = 0; channel < n_audio_channels; channel++)
            {
                if (sample_idx == delay_time)
                {
                    EXPECT_FLOAT_EQ(1.0f, result_buffer.channel(channel)[sample_idx]);
                }
                else
                {
                    EXPECT_FLOAT_EQ(0.0f, result_buffer.channel(channel)[sample_idx]);
                }
            }
        }
    }
}

constexpr int TEST_CHANNELS_STEREO = 2;

class TestStereoMixerPlugin : public ::testing::Test
{
protected:
    TestStereoMixerPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = std::make_unique<stereo_mixer_plugin::StereoMixerPlugin>(_host_control.make_host_control_mockup(TEST_SAMPLERATE));
        auto status = _module_under_test->init(TEST_SAMPLERATE);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
        _module_under_test->set_enabled(true);
        _module_under_test->set_event_output(&_fifo);
    }

    void WaitForStableParameters()
    {
        // Run one empty process call to update the smoothers to the current parameter values
        ChunkSampleBuffer temp_in_buffer(TEST_CHANNELS_STEREO);
        ChunkSampleBuffer temp_out_buffer(TEST_CHANNELS_STEREO);
        temp_in_buffer.clear();
        temp_out_buffer.clear();
        _module_under_test->process_audio(temp_in_buffer, temp_out_buffer);

        // Update smoothers until they are stationary
        while ((_module_under_test->_ch1_left_gain_smoother.stationary() &&
                _module_under_test->_ch1_right_gain_smoother.stationary() &&
                _module_under_test->_ch2_left_gain_smoother.stationary() &&
                _module_under_test->_ch2_right_gain_smoother.stationary()) == false)
        {
            _module_under_test->_ch1_left_gain_smoother.next_value();
            _module_under_test->_ch1_right_gain_smoother.next_value();
            _module_under_test->_ch2_left_gain_smoother.next_value();
            _module_under_test->_ch2_right_gain_smoother.next_value();
        }
    }

    HostControlMockup _host_control;
    std::unique_ptr<stereo_mixer_plugin::StereoMixerPlugin> _module_under_test;
    RtSafeRtEventFifo _fifo;
};

TEST_F(TestStereoMixerPlugin, TestInitialization)
{
    ASSERT_TRUE(_module_under_test.get());
    ASSERT_EQ("Stereo Mixer", _module_under_test->label());
    ASSERT_EQ("sushi.testing.stereo_mixer", _module_under_test->name());
}

TEST_F(TestStereoMixerPlugin, TestProcess)
{
    // Set up data
    int n_audio_channels = TEST_CHANNELS_STEREO;
    SampleBuffer<AUDIO_CHUNK_SIZE> input_buffer(n_audio_channels);
    SampleBuffer<AUDIO_CHUNK_SIZE> output_buffer(n_audio_channels);
    SampleBuffer<AUDIO_CHUNK_SIZE> expected_buffer(n_audio_channels);

    std::fill(input_buffer.channel(0), input_buffer.channel(0) + AUDIO_CHUNK_SIZE, 1.0f);
    std::fill(input_buffer.channel(1), input_buffer.channel(1) + AUDIO_CHUNK_SIZE, -2.0f);

    // Default configuration

    std::fill(expected_buffer.channel(0), expected_buffer.channel(0) + AUDIO_CHUNK_SIZE, 1.0f);
    std::fill(expected_buffer.channel(1), expected_buffer.channel(1) + AUDIO_CHUNK_SIZE, -2.0f);

    _module_under_test->process_audio(input_buffer, output_buffer);
    test_utils::compare_buffers<AUDIO_CHUNK_SIZE>(output_buffer, expected_buffer, 2);

    // Standard stereo throughput, right input channel inverted

    _module_under_test->_ch1_pan->set(0.0f);
    _module_under_test->_ch1_gain->set(0.791523611713336f);
    _module_under_test->_ch1_invert_phase->set(0.0f);
    _module_under_test->_ch2_pan->set(1.0f);
    _module_under_test->_ch2_gain->set(0.6944444444444444f);
    _module_under_test->_ch2_invert_phase->set(1.0f);

    std::fill(expected_buffer.channel(0), expected_buffer.channel(0) + AUDIO_CHUNK_SIZE, 0.5f);
    std::fill(expected_buffer.channel(1), expected_buffer.channel(1) + AUDIO_CHUNK_SIZE, 0.2f);

    WaitForStableParameters();

    _module_under_test->process_audio(input_buffer, output_buffer);
    test_utils::compare_buffers<AUDIO_CHUNK_SIZE>(output_buffer, expected_buffer, 2);

    // Inverted panning, left input channel inverted

    _module_under_test->_ch1_pan->set(1.0f);
    _module_under_test->_ch1_gain->set(0.8118191722242023f);
    _module_under_test->_ch1_invert_phase->set(1.0f);
    _module_under_test->_ch2_pan->set(0.0f);
    _module_under_test->_ch2_gain->set(0.7607112853777309f);
    _module_under_test->_ch2_invert_phase->set(0.0f);

    std::fill(expected_buffer.channel(0), expected_buffer.channel(0) + AUDIO_CHUNK_SIZE, -0.6f);
    std::fill(expected_buffer.channel(1), expected_buffer.channel(1) + AUDIO_CHUNK_SIZE, -0.7f);

    WaitForStableParameters();

    _module_under_test->process_audio(input_buffer, output_buffer);
    test_utils::compare_buffers<AUDIO_CHUNK_SIZE>(output_buffer, expected_buffer, 2);

    // Mono summing
    _module_under_test->_ch1_pan->set(0.5f);
    _module_under_test->_ch1_gain->set(0.8333333333333334f);
    _module_under_test->_ch1_invert_phase->set(0.0f);
    _module_under_test->_ch2_pan->set(0.5f);
    _module_under_test->_ch2_gain->set(0.8333333333333334f);
    _module_under_test->_ch2_invert_phase->set(0.0f);

    std::fill(expected_buffer.channel(0), expected_buffer.channel(0) + AUDIO_CHUNK_SIZE, -0.707946f);
    std::fill(expected_buffer.channel(1), expected_buffer.channel(1) + AUDIO_CHUNK_SIZE, -0.707946f);

    WaitForStableParameters();

    _module_under_test->process_audio(input_buffer, output_buffer);
    test_utils::compare_buffers<AUDIO_CHUNK_SIZE>(output_buffer, expected_buffer, 2);

    // Pan law test
    _module_under_test->_ch1_pan->set(0.35f);
    _module_under_test->_ch1_gain->set(0.8333333333333334f);
    _module_under_test->_ch1_invert_phase->set(0.0f);
    _module_under_test->_ch2_pan->set(0.9f);
    _module_under_test->_ch2_gain->set(0.8333333333333334f);
    _module_under_test->_ch2_invert_phase->set(0.0f);

    std::fill(expected_buffer.channel(0), expected_buffer.channel(0) + AUDIO_CHUNK_SIZE, 0.7955587392184001f + -0.28317642241051433f);
    std::fill(expected_buffer.channel(1), expected_buffer.channel(1) + AUDIO_CHUNK_SIZE, 0.49555873921840016f + -1.8831764224105143f);

    WaitForStableParameters();

    _module_under_test->process_audio(input_buffer, output_buffer);
    test_utils::compare_buffers<AUDIO_CHUNK_SIZE>(output_buffer, expected_buffer, 2);
}
