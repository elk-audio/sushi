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
constexpr int CV_CHANNELS = 0;


class TestPortAudioFrontend : public ::testing::Test
{
protected:
    TestPortAudioFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new PortAudioFrontend(&_engine);
    }

    void TearDown()
    {
        _module_under_test->cleanup();
        delete _module_under_test;
    }

    EngineMockup _engine{SAMPLE_RATE};
    PortAudioFrontend* _module_under_test;
};

TEST_F(TestPortAudioFrontend, TestInitSuccess)
{
    PaError init_value = PaErrorCode::paNoError;
    int device_count = 2;
    PortAudioFrontendConfiguration config(0,1,1,1);
    PaDeviceInfo expected_info;

    EXPECT_CALL(mockPortAudio, Pa_Initialize).WillOnce(Return(init_value));
    EXPECT_CALL(mockPortAudio, Pa_GetDeviceCount).WillOnce(Return(device_count));
    EXPECT_CALL(mockPortAudio, Pa_GetDeviceInfo(config.input_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    EXPECT_CALL(mockPortAudio, Pa_GetDeviceInfo(config.output_device_id.value())).WillOnce(Return(const_cast<const PaDeviceInfo*>(&expected_info)));
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);
}


