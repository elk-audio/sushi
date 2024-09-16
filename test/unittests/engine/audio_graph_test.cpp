#include <thread>

#include "gtest/gtest.h"

#include "engine/transport.h"

#include "engine/audio_graph.cpp"
#include "test_utils/host_control_mockup.h"
#include "test_utils/audio_graph_accessor.h"

constexpr float SAMPLE_RATE = 44000;
constexpr int TEST_MAX_TRACKS = 2;

using namespace sushi;
using namespace sushi::internal;
using namespace sushi::internal::engine;

#if defined(_MSC_VER)
#define DISABLE_MULTICORE_UNIT_TESTS
#endif


class TestAudioGraph : public ::testing::Test
{
protected:
    using ::testing::Test::SetUp; // Hide error of hidden overload of virtual function in clang when signatures differ but the name is the same
    TestAudioGraph() = default;

    void SetUp(int cores)
    {
        _module_under_test = std::make_unique<AudioGraph>(cores, TEST_MAX_TRACKS, SAMPLE_RATE, "");
        _accessor = std::make_unique<AudioGraphAccessor>(*_module_under_test);
    }

    HostControlMockup _hc;
    std::unique_ptr<AudioGraph> _module_under_test;

    std::unique_ptr<AudioGraphAccessor> _accessor;

    performance::PerformanceTimer _timer;

    Track _track_1 {_hc.make_host_control_mockup(SAMPLE_RATE), 2, &_timer};
    Track _track_2 {_hc.make_host_control_mockup(SAMPLE_RATE), 2, &_timer};
};

TEST_F(TestAudioGraph, TestSingleCoreOperation)
{
    SetUp(1);
    ASSERT_TRUE(_module_under_test->add(&_track_1));
    ASSERT_TRUE(_module_under_test->add(&_track_2));

    ASSERT_EQ(1u, _accessor->audio_graph().size());
    ASSERT_EQ(2u, _accessor->audio_graph()[0].size());

    _module_under_test->render();

    ASSERT_TRUE(_module_under_test->remove(&_track_1));
    ASSERT_TRUE(_module_under_test->remove(&_track_2));
    ASSERT_FALSE(_module_under_test->remove(&_track_2));

    ASSERT_EQ(0u, _accessor->audio_graph()[0].size());
}

/**
 * On Apple computers, if Twine is built with CoreAudio support, this unit test will fail,
 * since it fails to join a real-time thread workgroup - since there isn't one.
 * To re-enable the test, we'll first need to build Twine statically only for these test,
 * and either mock CoreAudio, or disable it in this static Twine build for these tests.
 *
 * It can also fail on Linux in other cases (e.g. sometimes when running in a Dockerized container).
 */
#ifndef DISABLE_MULTICORE_UNIT_TESTS
TEST_F(TestAudioGraph, TestMultiCoreOperation)
{
    SetUp(3);
    ASSERT_TRUE(_module_under_test->add(&_track_1));
    ASSERT_TRUE(_module_under_test->add(&_track_2));

    // Tracks should end up in slot 0 and 1
    ASSERT_EQ(3u, _accessor->audio_graph().size());
    ASSERT_EQ(1u, _accessor->audio_graph()[0].size());
    ASSERT_EQ(1u, _accessor->audio_graph()[1].size());
    ASSERT_EQ(0u, _accessor->audio_graph()[2].size());

    auto event = RtEvent::make_note_on_event(_track_1.id(), 0, 0, 48, 1.0f);
    _track_1.process_event(event);
    _track_2.process_event(event);
    _module_under_test->render();

    // Test that events were properly passed through
    auto queues = _module_under_test->event_outputs();
    EXPECT_EQ(1, queues[0].size());
    EXPECT_EQ(1, queues[1].size());
    EXPECT_EQ(0, queues[2].size());
}
#endif

TEST_F(TestAudioGraph, TestMaxNumberOfTracks)
{
    SetUp(1);
    ASSERT_TRUE(_module_under_test->add(&_track_1));
    ASSERT_TRUE(_module_under_test->add(&_track_2));
    ASSERT_FALSE(_module_under_test->add(&_track_2));

    ASSERT_EQ(1u, _accessor->audio_graph().size());
    ASSERT_EQ(2u, _accessor->audio_graph()[0].size());
}
