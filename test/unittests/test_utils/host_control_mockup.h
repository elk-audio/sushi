#ifndef SUSHI_HOST_CONTROL_MOCKUP_H
#define SUSHI_HOST_CONTROL_MOCKUP_H

#include "engine/host_control.h"
#include "test_utils/engine_mockup.h"

constexpr float DEFAULT_TEST_SAMPLERATE = 44100;
// Dummy host control object for testing Processors with direct access
// to a Transport object and a Dummy Event Dispatcher
class HostControlMockup
{
public:
    // Get a HostControl object with dummy dispatcher and transport members
    sushi::HostControl make_host_control_mockup(float sample_rate = DEFAULT_TEST_SAMPLERATE)
    {
        _transport.set_sample_rate(sample_rate);
        return sushi::HostControl(&_dummy_dispatcher, &_transport, &_plugin_library);
    }
    RtEventFifo<10>       _event_output;
    engine::Transport     _transport{DEFAULT_TEST_SAMPLERATE, &_event_output};
    engine::PluginLibrary _plugin_library;
    EventDispatcherMockup _dummy_dispatcher;
};

#endif //SUSHI_HOST_CONTROL_MOCKUP_H
