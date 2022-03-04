#include "gtest/gtest.h"

#include "engine/audio_engine.h"
#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "engine/controller/session_controller.cpp"

using namespace sushi;
using namespace sushi::midi_dispatcher;
using namespace sushi::engine;
using namespace sushi::engine::controller_impl;

constexpr float TEST_SAMPLE_RATE = 44100;

class SessionControllerTest : public ::testing::Test
{
protected:
    SessionControllerTest() {}
    void SetUp()
    {
        _audio_engine = std::make_unique<AudioEngine>(TEST_SAMPLE_RATE, 1, new EventDispatcherMockup());
        _event_dispatcher_mockup = static_cast<EventDispatcherMockup*>(_audio_engine->event_dispatcher());
        _midi_dispatcher = std::make_unique<MidiDispatcher>(_event_dispatcher_mockup);
        _module_under_test = std::make_unique<SessionController>(_audio_engine.get(), _midi_dispatcher.get());

        _audio_engine->set_audio_input_channels(8);
        _audio_engine->set_audio_output_channels(8);

        _audio_engine->create_track("Track 1", 2);
        _track_id = _audio_engine->processor_container()->track("Track 1")->id();
    }

    void TearDown() {}

    EventDispatcherMockup*                _event_dispatcher_mockup{nullptr};
    std::unique_ptr<AudioEngine>          _audio_engine;
    std::unique_ptr<MidiDispatcher>       _midi_dispatcher;
    std::unique_ptr<SessionController>    _module_under_test;
    int _track_id;
};

TEST_F(SessionControllerTest, TestGettingProcessors)
{

}