/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/test_utils.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "audio_frontends/reactive_frontend.h"

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::audio_frontend;

constexpr float SAMPLE_RATE = 44100;

// Engine mockup that copies the input ControlBuffer to the output ControlBuffer during
// process_chunk. Lets us verify that CV / gate values set through set_cv_input() /
// set_gate_input() reach the engine and that cv_output() / gate_output() reflect
// anything the engine writes to the output ControlBuffer.
class CvLoopbackEngineMockup : public EngineMockup
{
public:
    explicit CvLoopbackEngineMockup(float sample_rate) : EngineMockup(sample_rate) {}

    void process_chunk(ChunkSampleBuffer* in_buffer,
                       ChunkSampleBuffer* out_buffer,
                       engine::ControlBuffer* in_controls,
                       engine::ControlBuffer* out_controls,
                       Time timestamp,
                       int64_t sample_count) override
    {
        EngineMockup::process_chunk(in_buffer, out_buffer, in_controls, out_controls,
                                    timestamp, sample_count);
        *out_controls = *in_controls;
    }
};

// Engine mockup that records the arguments of set_output_latency() and
// notify_interrupted_audio() — both are swallowed by the plain EngineMockup.
class RecordingEngineMockup : public EngineMockup
{
public:
    explicit RecordingEngineMockup(float sample_rate) : EngineMockup(sample_rate) {}

    void set_output_latency(Time latency) override
    {
        output_latency = latency;
    }

    void notify_interrupted_audio(Time duration) override
    {
        interruption_notified = true;
        interruption_duration = duration;
    }

    Time output_latency {0};
    bool interruption_notified {false};
    Time interruption_duration {0};
};

class TestReactiveFrontend : public ::testing::Test
{
protected:
    TestReactiveFrontend() = default;

    void SetUp() override
    {
        _module_under_test = std::make_unique<ReactiveFrontend>(&_engine);
    }

    EngineMockup _engine {SAMPLE_RATE};
    std::unique_ptr<ReactiveFrontend> _module_under_test;
};

TEST_F(TestReactiveFrontend, TestInitConfiguresChannelsAndLatency)
{
    ReactiveFrontendConfiguration config(/*audio_in*/ 4, /*audio_out*/ 8,
                                         /*cv_in*/ 2, /*cv_out*/ 1,
                                         /*output_latency_us*/ 1500);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    EXPECT_EQ(4, _engine.audio_input_channels());
    EXPECT_EQ(8, _engine.audio_output_channels());
    EXPECT_EQ(2, _engine.cv_input_channels());
    EXPECT_EQ(1, _engine.cv_output_channels());
}

TEST_F(TestReactiveFrontend, TestInitAcceptsMaxChannels)
{
    ReactiveFrontendConfiguration config(MAX_REACTIVE_CHANNELS, MAX_REACTIVE_CHANNELS, 0, 0);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    EXPECT_EQ(MAX_REACTIVE_CHANNELS, _engine.audio_input_channels());
    EXPECT_EQ(MAX_REACTIVE_CHANNELS, _engine.audio_output_channels());
}

TEST_F(TestReactiveFrontend, TestInitRejectsTooManyInputChannels)
{
    ReactiveFrontendConfiguration config(MAX_REACTIVE_CHANNELS + 1, 2, 0, 0);
    EXPECT_EQ(AudioFrontendStatus::INVALID_N_CHANNELS, _module_under_test->init(&config));
}

TEST_F(TestReactiveFrontend, TestInitRejectsTooManyOutputChannels)
{
    ReactiveFrontendConfiguration config(2, MAX_REACTIVE_CHANNELS + 1, 0, 0);
    EXPECT_EQ(AudioFrontendStatus::INVALID_N_CHANNELS, _module_under_test->init(&config));
}

TEST_F(TestReactiveFrontend, TestUpdateSampleRatePropagatesToEngine)
{
    ReactiveFrontendConfiguration config(2, 2, 0, 0);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    _module_under_test->update_sample_rate(48000.0f);
    EXPECT_FLOAT_EQ(48000.0f, _engine.sample_rate());
}

TEST_F(TestReactiveFrontend, TestProcessAudioClearsOutputAndCallsEngine)
{
    ReactiveFrontendConfiguration config(2, 2, 0, 0);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 0.25f);
    test_utils::fill_sample_buffer(out_buffer, -1.0f);  // garbage the frontend must clear

    ASSERT_FALSE(_engine.process_called);
    _module_under_test->process_audio(in_buffer, out_buffer, 0, std::chrono::microseconds(0));
    ASSERT_TRUE(_engine.process_called);

    // EngineMockup's process_chunk copies in → out, so after clear+process we see the input.
    test_utils::assert_buffer_value(0.25f, out_buffer);
}

class TestReactiveFrontendCv : public ::testing::Test
{
protected:
    TestReactiveFrontendCv() = default;

    void SetUp() override
    {
        _module_under_test = std::make_unique<ReactiveFrontend>(&_engine);
        ReactiveFrontendConfiguration config(2, 2, 2, 2);
        ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));
    }

    CvLoopbackEngineMockup _engine {SAMPLE_RATE};
    std::unique_ptr<ReactiveFrontend> _module_under_test;
};

TEST_F(TestReactiveFrontendCv, TestCvInputReachesEngineAndCvOutputIsReadable)
{
    _module_under_test->set_cv_input(0, 0.75f);
    _module_under_test->set_cv_input(1, 0.125f);

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    _module_under_test->process_audio(in_buffer, out_buffer, 0, std::chrono::microseconds(0));

    // The loopback engine forwards in_controls to out_controls, so cv_output() should
    // now reflect what we wrote with set_cv_input().
    EXPECT_FLOAT_EQ(0.75f,  _module_under_test->cv_output(0));
    EXPECT_FLOAT_EQ(0.125f, _module_under_test->cv_output(1));
}

TEST_F(TestReactiveFrontendCv, TestGateInputReachesEngineAndGateOutputIsReadable)
{
    _module_under_test->set_gate_input(0, true);
    _module_under_test->set_gate_input(3, true);
    _module_under_test->set_gate_input(7, false);

    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    _module_under_test->process_audio(in_buffer, out_buffer, 0, std::chrono::microseconds(0));

    EXPECT_TRUE (_module_under_test->gate_output(0));
    EXPECT_TRUE (_module_under_test->gate_output(3));
    EXPECT_FALSE(_module_under_test->gate_output(7));
    EXPECT_FALSE(_module_under_test->gate_output(15));  // never written
}

class TestReactiveFrontendRecording : public ::testing::Test
{
protected:
    TestReactiveFrontendRecording() = default;

    void SetUp() override
    {
        _module_under_test = std::make_unique<ReactiveFrontend>(&_engine);
    }

    RecordingEngineMockup _engine {SAMPLE_RATE};
    std::unique_ptr<ReactiveFrontend> _module_under_test;
};

TEST_F(TestReactiveFrontendRecording, TestInitForwardsOutputLatencyToEngine)
{
    ReactiveFrontendConfiguration config(2, 2, 0, 0, /*output_latency_us*/ 2900);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    EXPECT_EQ(std::chrono::microseconds(2900), _engine.output_latency);
}

TEST_F(TestReactiveFrontendRecording, TestInitDefaultLatencyIsZero)
{
    ReactiveFrontendConfiguration config(2, 2, 0, 0);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    EXPECT_EQ(std::chrono::microseconds(0), _engine.output_latency);
}

TEST_F(TestReactiveFrontendRecording, TestNotifyInterruptedAudioForwardsToEngine)
{
    ReactiveFrontendConfiguration config(2, 2, 0, 0);
    ASSERT_EQ(AudioFrontendStatus::OK, _module_under_test->init(&config));

    ASSERT_FALSE(_engine.interruption_notified);

    auto duration = std::chrono::milliseconds(42);
    _module_under_test->notify_interrupted_audio(duration);

    EXPECT_TRUE(_engine.interruption_notified);
    EXPECT_EQ(std::chrono::duration_cast<Time>(duration), _engine.interruption_duration);
}
