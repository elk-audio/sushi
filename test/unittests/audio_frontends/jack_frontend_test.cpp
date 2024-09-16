#include <functional>

#include "gtest/gtest.h"

#include "test_utils/engine_mockup.h"
#include "test_utils/jack_mockup.cpp"
#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.cpp"
#endif
#include "engine/midi_dispatcher.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

#include "audio_frontends/jack_frontend.cpp"

using ::testing::internal::posix::GetEnv;

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::audio_frontend;
using namespace sushi::internal::midi_dispatcher;

constexpr float SAMPLE_RATE = 44000;
constexpr int CV_CHANNELS = 0;

namespace sushi::internal::audio_frontend {
class JackFrontendAccessor
{
public:
    explicit JackFrontendAccessor(JackFrontend* f) : _friend(f) {}

    jack_client_t* client()
    {
        return _friend->_client;
    }

private:
    JackFrontend* _friend;
};
}

class TestJackFrontend : public ::testing::Test
{
protected:
    TestJackFrontend() = default;

    void SetUp() override
    {
        _module_under_test = std::make_unique<JackFrontend>(&_engine);
        _accessor = std::make_unique<JackFrontendAccessor>(_module_under_test.get());
    }

    void TearDown() override
    {
        _module_under_test->cleanup();
    }

    EngineMockup _engine {SAMPLE_RATE};

    std::unique_ptr<JackFrontend> _module_under_test;
    std::unique_ptr<JackFrontendAccessor> _accessor;
};

TEST_F(TestJackFrontend, TestOperation)
{
    JackFrontendConfiguration config("Jack Client", "Jack Server", false, CV_CHANNELS, CV_CHANNELS);
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendStatus::OK, ret_code);

    /* Can't call run directly because that will freeze the test due to the sleep() call*/
    jack_activate(_accessor->client());

    ASSERT_TRUE(_engine.process_called);
}


