#include <array>

#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/portaudio_mockup.cpp"

#define private public
#include "audio_frontends/portaudio_frontend.cpp"


using ::testing::internal::posix::GetEnv;
using ::testing::Return;

using namespace sushi;
using namespace sushi::audio_frontend;
using namespace sushi::midi_dispatcher;

constexpr float SAMPLE_RATE = 44000;


class TestPortAudioFrontend : public ::testing::Test
{
protected:
    TestPortAudioFrontend()
    {
    }

    void SetUp()
    {
        mockPortAudio = new MockPortAudio();
        _module_under_test = new PortAudioFrontend(&_engine);
    }

    void TearDown()
    {
        // Clean up
        EXPECT_CALL(*mockPortAudio, Pa_IsStreamActive).WillOnce(Return(1));
        EXPECT_CALL(*mockPortAudio, Pa_StopStream);
        delete _module_under_test;
        delete mockPortAudio;
    }

    EngineMockup _engine{SAMPLE_RATE};
    PortAudioFrontend* _module_under_test;
};

TEST_F(TestPortAudioFrontend, TestInitSuccess)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 2;
    PaDeviceInfo expected_info;
    PortAudioFrontendConfiguration config(0,1,1,1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultInputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultOutputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.input_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.output_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitFailOnPaInit)
{
    PaError init_value = PaErrorCode::paNotInitialized;
    PortAudioFrontendConfiguration config(0,1,1,1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::AUDIO_HW_ERROR, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitFailGetDeviceCount)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 0;
    PortAudioFrontendConfiguration config(0,1,1,1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::AUDIO_HW_ERROR, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitiFailSamplerate)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 2;
    PaDeviceInfo expected_info;
    PortAudioFrontendConfiguration config(0,1,1,1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultInputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultOutputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.input_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.output_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_IsFormatSupported).WillRepeatedly(Return(PaErrorCode::paInvalidSampleRate));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::AUDIO_HW_ERROR, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitiFailOpenStream)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 2;
    PaDeviceInfo expected_info;
    PortAudioFrontendConfiguration config(0,1,1,1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultInputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultOutputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.input_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.output_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_OpenStream).WillOnce(Return(PaErrorCode::paInvalidSampleRate));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::AUDIO_HW_ERROR, ret_code);
}

TEST_F(TestPortAudioFrontend, TestRun)
{
    EXPECT_CALL(*mockPortAudio, Pa_StartStream);
    _module_under_test->run();
    ASSERT_TRUE(_engine.realtime());
}

TEST_F(TestPortAudioFrontend, TestProcess)
{
    PortAudioFrontendConfiguration config(0,0,0,0);
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 1;
    PaDeviceInfo device_info;
    device_info.maxInputChannels = 1;
    device_info.maxOutputChannels = 1;

    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo).WillRepeatedly(Return(&device_info));
    auto result = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, result);

    constexpr int FRAME_SIZE = 64;
    std::array<float, FRAME_SIZE> input_data{1.0f};
    std::array<float, FRAME_SIZE> output_data{0.0f};
    PaStreamCallbackTimeInfo time_info;
    PaStreamCallbackFlags status_flags;
    PortAudioFrontend::rt_process_callback(static_cast<void*>(input_data.data()),
                                           static_cast<void*>(output_data.data()),
                                           FRAME_SIZE,
                                           &time_info,
                                           status_flags,
                                           static_cast<void*>(_module_under_test));
    ASSERT_EQ(input_data, output_data);
    ASSERT_TRUE(_engine.process_called);
}