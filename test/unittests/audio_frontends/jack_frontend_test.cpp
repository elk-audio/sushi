#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/jack_mockup.cpp"
#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.cpp"
#endif
#include "engine/midi_dispatcher.h"

#define private public
#include "audio_frontends/jack_frontend.cpp"


using ::testing::internal::posix::GetEnv;

using namespace sushi;
using namespace sushi::audio_frontend;
using namespace sushi::midi_dispatcher;

constexpr float SAMPLE_RATE = 44000;
constexpr int CV_CHANNELS = 0;


class TestJackFrontend : public ::testing::Test
{
protected:
    TestJackFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new JackFrontend(&_engine);
    }

    void TearDown()
    {
        _module_under_test->cleanup();
        delete _module_under_test;
    }

    EngineMockup _engine{SAMPLE_RATE};
    JackFrontend* _module_under_test;
};

TEST_F(TestJackFrontend, TestOperation)
{
    JackFrontendConfiguration config("Jack Client", "Jack Server", false, CV_CHANNELS, CV_CHANNELS);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);

    /* Can't call run directly because that will freeze the test due to the sleep() call*/
    jack_activate(_module_under_test->_client);

    ASSERT_TRUE(_engine.process_called);
}


