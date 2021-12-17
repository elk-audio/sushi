#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/portaudio_mockup.cpp"

#define private public
#include "audio_frontends/portaudio_frontend.cpp"


using ::testing::internal::posix::GetEnv;

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

TEST_F(TestPortAudioFrontend, TestOperation)
{
    PortAudioFrontendConfiguration config(1,1,1,1);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);
}


