#include "gtest/gtest.h"

#define private public
#include "jack_mockup.cpp"

#include "audio_frontends/jack_frontend.cpp"
#include "engine_mockup.h"
#include "control_frontends/alsa_midi_frontend.cpp"


using ::testing::internal::posix::GetEnv;

using namespace sushi;
using namespace sushi::audio_frontend;
using namespace sushi::midi_dispatcher;

static constexpr unsigned int SAMPLE_RATE = 44000;

class TestJackFrontend : public ::testing::Test
{
protected:
    TestJackFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new JackFrontend(&_engine, &_midi_dispatcher);
    }

    void TearDown()
    {
        _module_under_test->cleanup();
        delete _module_under_test;
    }

    EngineMockup _engine{SAMPLE_RATE};
    JackFrontend* _module_under_test;
    MidiDispatcher _midi_dispatcher{&_engine};
};

TEST_F(TestJackFrontend, TestOperation)
{
    JackFrontendConfiguration config("Jack Client", "Jack Server", false);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);

    /* Can't call run directly cause that will freeze the test due to the sleep() call*/
    jack_activate(_module_under_test->_client);

    ASSERT_TRUE(_engine.process_called);
}


