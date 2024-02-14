#include <array>

#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/portaudio_mockup.h"

#include <elk-warning-suppressor/warning_suppressor.hpp>

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
ELK_POP_WARNING

#include "audio_frontends/portaudio_frontend.cpp"


using ::testing::internal::posix::GetEnv;
using ::testing::Return;
using ::testing::NiceMock;
using ::testing::SetArgPointee;

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::audio_frontend;
using namespace sushi::internal::midi_dispatcher;

constexpr float SAMPLE_RATE = 44000;


class TestPortAudioFrontend : public ::testing::Test
{
protected:
    TestPortAudioFrontend()
    {
    }

    void SetUp() override
    {
        mockPortAudio = std::make_unique<NiceMock<MockPortAudio>>();
        _module_under_test = std::make_unique<PortAudioFrontend>(&_engine);
    }

    void TearDown() override
    {
        if (_module_under_test->_stream_initialized)
        {
            EXPECT_CALL(*mockPortAudio, Pa_IsStreamActive).WillOnce(Return(1));
            EXPECT_CALL(*mockPortAudio, Pa_StopStream);
        }
        EXPECT_CALL(*mockPortAudio, Pa_Terminate);
        _module_under_test.reset();
        mockPortAudio.reset();
    }

    EngineMockup _engine{SAMPLE_RATE};
    std::unique_ptr<PortAudioFrontend> _module_under_test;
};

TEST_F(TestPortAudioFrontend, TestInitSuccess)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 2;
    PaDeviceInfo expected_info;
    expected_info.maxInputChannels = 2;
    expected_info.maxOutputChannels = 2;
    PaStreamInfo stream_info;
    PortAudioFrontendConfiguration config(0, 1, 0.0f, 0.0f, 1, 1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultInputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultOutputDevice);
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.input_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo(config.output_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(*mockPortAudio, Pa_GetStreamInfo(_module_under_test->_stream)).WillOnce(Return(const_cast<const PaStreamInfo*>(&stream_info)));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitFailOnPaInit)
{
    PaError init_value = PaErrorCode::paNotInitialized;
    PortAudioFrontendConfiguration config(0, 1, 0.0f, 0.0f, 1, 1);

    EXPECT_CALL(*mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::AUDIO_HW_ERROR, ret_code);
}

TEST_F(TestPortAudioFrontend, TestInitFailGetDeviceCount)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 0;
    PortAudioFrontendConfiguration config(0, 1, 0.0f, 0.0f, 1, 1);

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
    expected_info.maxInputChannels = 2;
    expected_info.maxOutputChannels = 2;
    PortAudioFrontendConfiguration config(0, 1, 0.0f, 0.0f, 1, 1);

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
    expected_info.maxInputChannels = 2;
    expected_info.maxOutputChannels = 2;
    PortAudioFrontendConfiguration config(0, 1, 0.0f, 0.0f, 1, 1);

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
    PortAudioFrontendConfiguration config(0, 0, 0.0f, 0.0f, 0, 0);
    int device_count = 1;
    PaDeviceInfo device_info;
    device_info.maxInputChannels = 1;
    device_info.maxOutputChannels = 1;
    PaStreamInfo stream_info;


    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo).WillRepeatedly(Return(&device_info));
    EXPECT_CALL(*mockPortAudio, Pa_GetStreamInfo(_module_under_test->_stream)).WillOnce(Return(const_cast<const PaStreamInfo*>(&stream_info)));
    auto result = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, result);

    std::array<float, AUDIO_CHUNK_SIZE> input_data{1.0f};
    std::array<float, AUDIO_CHUNK_SIZE> output_data{0.0f};
    PaStreamCallbackTimeInfo time_info;
    PaStreamCallbackFlags status_flags = 0;
    PortAudioFrontend::rt_process_callback(static_cast<void*>(input_data.data()),
                                           static_cast<void*>(output_data.data()),
                                           AUDIO_CHUNK_SIZE,
                                           &time_info,
                                           status_flags,
                                           static_cast<void*>(_module_under_test.get()));
    ASSERT_EQ(input_data, output_data);
    ASSERT_TRUE(_engine.process_called);
}

TEST_F(TestPortAudioFrontend, TestGetDeviceName)
{
    auto expected_name = "a_device";
    PaDeviceInfo device_info;
    device_info.maxInputChannels = 1;
    device_info.maxOutputChannels = 1;
    device_info.name = expected_name;

    EXPECT_CALL(*mockPortAudio, Pa_GetDeviceInfo)
    .WillOnce(Return(&device_info))
    .WillOnce(Return(&device_info))
    .WillOnce(Return(nullptr));

    std::optional<int> portaudio_output_device_id = 1;

    auto device_name = sushi::internal::audio_frontend::get_portaudio_output_device_name(portaudio_output_device_id);

    ASSERT_EQ(device_name, expected_name); // The specified device

    EXPECT_CALL(*mockPortAudio, Pa_GetDefaultOutputDevice);

    device_name = sushi::internal::audio_frontend::get_portaudio_output_device_name(std::nullopt);

    ASSERT_EQ(device_name, expected_name); // The default device

    device_name = sushi::internal::audio_frontend::get_portaudio_output_device_name(4);

    EXPECT_FALSE(device_name.has_value());
}
