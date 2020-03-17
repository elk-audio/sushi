#include <algorithm>
#include <thread>

#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "engine/transport.h"

#define private public
#define protected public

#include "engine/audio_engine.cpp"
#include "test_utils/dummy_processor.h"
#include "test_utils/host_control_mockup.h"

constexpr float SAMPLE_RATE = 44000;
constexpr int TEST_CHANNEL_COUNT = 4;
using namespace sushi;
using namespace sushi::engine;

/*
* Engine tests
*/
class TestClipDetector : public ::testing::Test
{
protected:
    TestClipDetector()
    {}

    void SetUp()
    {
        _module_under_test.set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test.set_output_channels(TEST_CHANNEL_COUNT);
    }

    ClipDetector _module_under_test{SAMPLE_RATE};
};

TEST_F(TestClipDetector, TestClipping)
{
    RtSafeRtEventFifo queue;
    ChunkSampleBuffer buffer(TEST_CHANNEL_COUNT);
    test_utils::fill_sample_buffer(buffer, 0.5f);
    _module_under_test.detect_clipped_samples(buffer, queue, false);
    /* No samples outside (-1.0, 1.0) so this should result in no notifications */
    ASSERT_TRUE(queue.empty());

    /* Set 2 samples to clipped, we should now have 2 clip notifications */
    buffer.channel(1)[10] = 1.5f;
    buffer.channel(3)[6] = -1.3f;
    _module_under_test.detect_clipped_samples(buffer, queue, false);
    ASSERT_FALSE(queue.empty());
    RtEvent notification;
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_EQ(1, notification.clip_notification_event()->channel());
    ASSERT_EQ(ClipNotificationRtEvent::ClipChannelType::OUTPUT, notification.clip_notification_event()->channel_type());
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_EQ(3, notification.clip_notification_event()->channel());
    ASSERT_EQ(ClipNotificationRtEvent::ClipChannelType::OUTPUT, notification.clip_notification_event()->channel_type());

    /* But calling again immediately should not trigger due to the rate limiting */
    _module_under_test.detect_clipped_samples(buffer, queue, false);
    ASSERT_TRUE(queue.empty());

    /* But calling with audio_inout set to true should trigger 2 new */
    _module_under_test.detect_clipped_samples(buffer, queue, true);
    ASSERT_FALSE(queue.empty());
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_EQ(ClipNotificationRtEvent::ClipChannelType::INPUT, notification.clip_notification_event()->channel_type());
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_FALSE(queue.pop(notification));
}

class TestProcessorContainer : public ::testing::Test
{
protected:
    TestProcessorContainer() {}

    HostControlMockup  _hc;
    ProcessorContainer _module_under_test;
};

TEST_F(TestProcessorContainer, TestAddingAndRemoving)
{
    auto proc_1 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_1->set_name("one");
    auto proc_2 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_2->set_name("two");
    auto id_1 = proc_1->id();
    auto id_2 = proc_2->id();
    ASSERT_TRUE(_module_under_test.add_processor(proc_1));
    ASSERT_TRUE(_module_under_test.add_processor(proc_2));

    // Assert false when adding proc_2 again
    ASSERT_FALSE(_module_under_test.add_processor(proc_2));

    // Access these processors
    ASSERT_TRUE(_module_under_test.processor_exists(id_1));
    ASSERT_TRUE(_module_under_test.processor_exists("two"));
    ASSERT_EQ("one", _module_under_test.processor("one")->name());
    ASSERT_EQ(id_2, _module_under_test.processor(id_2)->id());
    ASSERT_EQ(proc_2, _module_under_test.mutable_processor(id_2));
    ASSERT_EQ(proc_1, _module_under_test.mutable_processor(id_1));
    ASSERT_EQ(2u, _module_under_test.all_processors().size());

    // Access non-existing processors
    ASSERT_FALSE(_module_under_test.processor_exists(ObjectId(123)));
    ASSERT_FALSE(_module_under_test.processor_exists("three"));
    ASSERT_EQ(nullptr, _module_under_test.processor("four"));
    ASSERT_EQ(nullptr, _module_under_test.processor(ObjectId(234)));

    // Remove processors
    ASSERT_TRUE(_module_under_test.remove_processor(id_1));
    ASSERT_TRUE(_module_under_test.remove_processor(id_2));
    ASSERT_FALSE(_module_under_test.remove_processor(id_1));

    ASSERT_FALSE(_module_under_test.processor_exists(id_1));
    ASSERT_FALSE(_module_under_test.processor_exists("two"));
    ASSERT_EQ(nullptr, _module_under_test.mutable_processor(id_2));
    ASSERT_EQ(nullptr, _module_under_test.mutable_processor(id_1));
}


class TestAudioGraph : public ::testing::Test
{
protected:
    TestAudioGraph() {}

    void SetUp(int cores)
    {
        _module_under_test = std::make_unique<AudioGraph>(cores);
    }

    HostControlMockup             _hc;
    std::unique_ptr<AudioGraph>   _module_under_test;
    performance::PerformanceTimer _timer;
    Track                         _track_1{_hc.make_host_control_mockup(SAMPLE_RATE), 2, &_timer};
    Track                         _track_2{_hc.make_host_control_mockup(SAMPLE_RATE), 2, &_timer};
};

TEST_F(TestAudioGraph, TestSingleCoreOperation)
{
    SetUp(1);
    ASSERT_TRUE(_module_under_test->add(&_track_1));
    ASSERT_TRUE(_module_under_test->add(&_track_2));

    ASSERT_EQ(1u, _module_under_test->_audio_graph.size());
    ASSERT_EQ(2u, _module_under_test->_audio_graph[0].size());

    _module_under_test->render();

    ASSERT_TRUE(_module_under_test->remove(&_track_1));
    ASSERT_TRUE(_module_under_test->remove(&_track_2));
    ASSERT_FALSE(_module_under_test->remove(&_track_2));

    ASSERT_EQ(0u, _module_under_test->_audio_graph[0].size());
}

TEST_F(TestAudioGraph, TestMultiCoreOperation)
{
    SetUp(3);
    ASSERT_TRUE(_module_under_test->add(&_track_1));
    ASSERT_TRUE(_module_under_test->add(&_track_2));

    // Tracks should end up in slot 0 and 1
    ASSERT_EQ(3u, _module_under_test->_audio_graph.size());
    ASSERT_EQ(1u, _module_under_test->_audio_graph[0].size());
    ASSERT_EQ(1u, _module_under_test->_audio_graph[1].size());
    ASSERT_EQ(0u, _module_under_test->_audio_graph[2].size());

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

TEST_F(TestProcessorContainer, TestTrackManagement)
{
    auto proc_1 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_1->set_name("one");
    auto proc_2 = std::make_shared<DummyProcessor>(_hc.make_host_control_mockup(SAMPLE_RATE));
    proc_2->set_name("two");
    auto track = std::make_shared<Track>(_hc.make_host_control_mockup(SAMPLE_RATE), 2, nullptr);
    track->set_name("track");

    ASSERT_TRUE(_module_under_test.add_processor(proc_1));
    ASSERT_TRUE(_module_under_test.add_processor(proc_2));
    ASSERT_TRUE(_module_under_test.add_processor(track));

    ASSERT_TRUE(_module_under_test.add_track(track));
    ASSERT_FALSE(_module_under_test.add_track(track));

    ASSERT_TRUE(_module_under_test.add_to_track(proc_1, track->id(), std::nullopt));
    ASSERT_TRUE(_module_under_test.add_to_track(proc_2, track->id(), proc_1->id()));

    ASSERT_TRUE(_module_under_test.processor_exists(track->id()));
    ASSERT_EQ(track, _module_under_test.track(track->id()));
    ASSERT_EQ(track, _module_under_test.track("track"));
    ASSERT_EQ(nullptr, _module_under_test.track("two"));

    auto procs = _module_under_test.processors_on_track(track->id());
    ASSERT_EQ(2u, procs.size());
    ASSERT_EQ("two", procs[0]->name());
    ASSERT_EQ("one", procs[1]->name());

    ASSERT_TRUE(_module_under_test.remove_from_track(proc_2->id(), track->id()));
    procs = _module_under_test.processors_on_track(track->id());
    ASSERT_EQ(1u, procs.size());
    ASSERT_EQ("one", procs[0]->name());

    ASSERT_TRUE(_module_under_test.remove_from_track(proc_1->id(), track->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(proc_1->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(proc_2->id()));
    ASSERT_TRUE(_module_under_test.remove_track(track->id()));
    ASSERT_TRUE(_module_under_test.remove_processor(track->id()));

    ASSERT_TRUE(_module_under_test.all_tracks().empty());
    ASSERT_FALSE(_module_under_test.processor_exists("track"));
    ASSERT_FALSE(_module_under_test.processor_exists("one"));
    ASSERT_FALSE(_module_under_test.processor_exists("two"));
}

/*
* Engine tests
*/
class TestEngine : public ::testing::Test
{
protected:
    TestEngine() {}

    void SetUp()
    {
        _module_under_test = std::make_unique<AudioEngine>(SAMPLE_RATE, 1);
        _module_under_test->set_audio_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_audio_output_channels(TEST_CHANNEL_COUNT);
        _processors = _module_under_test->processor_container();
    }

    std::unique_ptr<AudioEngine> _module_under_test;
    const BaseProcessorContainer* _processors;
};

/*
 * Test that 1:s in gives 1:s out
 */
TEST_F(TestEngine, TestProcess)
{
    /* Add a plugin track and connect it to inputs and outputs */
    auto [status, track_id] = _module_under_test->create_track("test_track", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto track = _module_under_test->processor_container()->track("test_track");
    ASSERT_NE(nullptr, track);

    auto res = _module_under_test->connect_audio_input_bus(0, 0, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, res);
    res = _module_under_test->connect_audio_output_bus(0, 0, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, res);

    /* Run tests */
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    ControlBuffer control_buffer;
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    test_utils::fill_sample_buffer(out_buffer, 0.5f);

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);

    /* Separate the first 2 channels, which should pass through unprocessed
     * and the 2 last, which should be set to 0 since they are not connected to anything */
    auto main_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);
    auto second_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 2, 2);

    test_utils::assert_buffer_value(1.0f, main_bus, test_utils::DECIBEL_ERROR);
    test_utils::assert_buffer_value(0.0f, second_bus, test_utils::DECIBEL_ERROR);

    /* Add a plugin to the track and do the same thing */
    auto [load_status, plugin_id]  = _module_under_test->load_plugin("sushi.testing.gain", "gain", "", PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, load_status);
    res = _module_under_test->add_plugin_to_track(plugin_id, track->id());
    ASSERT_EQ(EngineReturnStatus::OK, res);

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);
    main_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);

    test_utils::assert_buffer_value(1.0f, main_bus, test_utils::DECIBEL_ERROR);
}

TEST_F(TestEngine, TestOutputMixing)
{
    auto [status_1, track_1_id] = _module_under_test->create_track("1", 2);
    auto [status_2, track_2_id] = _module_under_test->create_track("2", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status_1);
    ASSERT_EQ(EngineReturnStatus::OK, status_2);
    _module_under_test->connect_audio_input_bus(0, 0, track_1_id);
    _module_under_test->connect_audio_input_bus(1, 0, track_2_id);
    _module_under_test->connect_audio_output_bus(0, 0, track_1_id);
    _module_under_test->connect_audio_output_bus(0, 0, track_2_id);

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    ControlBuffer control_buffer;

    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);

    /* Both track's outputs are routed to bus 0, so they should sum to 2 */
    auto main_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);

    test_utils::assert_buffer_value(2.0f, main_bus, test_utils::DECIBEL_ERROR);
}

TEST_F(TestEngine, TestCreateEmptyTrack)
{
    auto [status, track_id] = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_TRUE(_processors->processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0].size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0][0]->name(),"left");

    /* Test invalid name */
    std::tie(status, track_id) = _module_under_test->create_track("left", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, track_id) = _module_under_test->create_track("", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);

    /* Test removal */
    status = _module_under_test->delete_track("left");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_FALSE(_processors->processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0].size(),0u);

    /* Test invalid number of channels */
    std::tie(status, track_id) = _module_under_test->create_track("left", 3);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
}

TEST_F(TestEngine, TestAddAndRemovePlugin)
{
    /* Test adding Internal plugins */
    auto [left_track_status, left_track_id] = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, left_track_status);
    auto [gain_status, gain_id] = _module_under_test->load_plugin("sushi.testing.gain",
                                                                  "gain",
                                                                  "   ",
                                                                  PluginType::INTERNAL);
    ASSERT_EQ(gain_status, EngineReturnStatus::OK);
    auto [synth_status, synth_id] = _module_under_test->load_plugin("sushi.testing.sampleplayer",
                                                                    "synth",
                                                                    "",
                                                                    PluginType::INTERNAL);
    ASSERT_EQ(synth_status, EngineReturnStatus::OK);

    auto status = _module_under_test->add_plugin_to_track(gain_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    /* Add synth before gain */
    status = _module_under_test->add_plugin_to_track(synth_id, left_track_id, gain_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    auto processors = _processors->processors_on_track(left_track_id);
    ASSERT_EQ(2u, processors.size());
    ASSERT_EQ("synth", processors.front()->name());

    /* Check that processors exists and in the right order on track "main" */
    ASSERT_TRUE(_processors->processor_exists("gain"));
    ASSERT_TRUE(_processors->processor_exists("synth"));
    ASSERT_EQ(2u, _module_under_test->_audio_graph._audio_graph[0][0]->_processors.size());
    ASSERT_EQ("synth", _module_under_test->_audio_graph._audio_graph[0][0]->_processors[0]->name());
    ASSERT_EQ("gain", _module_under_test->_audio_graph._audio_graph[0][0]->_processors[1]->name());

    /* Move a processor from 1 track to another */
    auto [right_track_status, right_track_id] = _module_under_test->create_track("right", 2);
    ASSERT_EQ(EngineReturnStatus::OK, right_track_status);
    status = _module_under_test->remove_plugin_from_track(synth_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->add_plugin_to_track(synth_id, right_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto left_processors = _processors->processors_on_track(left_track_id);
    auto right_processors = _processors->processors_on_track(right_track_id);
    ASSERT_EQ(1u, left_processors.size());
    ASSERT_EQ("gain", left_processors.front()->name());
    ASSERT_EQ(1u, right_processors.size());
    ASSERT_EQ("synth", right_processors.front()->name());

    /* Test removing plugin */
    status = _module_under_test->remove_plugin_from_track(gain_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    processors = _processors->processors_on_track(left_track_id);
    ASSERT_EQ(0u, processors.size());

    status = _module_under_test->delete_plugin(gain_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ASSERT_FALSE(_processors->processor_exists("gain"));
    ASSERT_EQ(0u, _module_under_test->_audio_graph._audio_graph[0][0]->_processors.size());
    ASSERT_EQ("synth", _module_under_test->_audio_graph._audio_graph[0][1]->_processors[0]->name());

    /* Negative tests */
    ObjectId id;
    std::tie(status, id) = _module_under_test->load_plugin("sushi.testing.passthrough",
                                                           "dummyname",
                                                           "",
                                                           PluginType::INTERNAL);
    status = _module_under_test->add_plugin_to_track(ObjectId(123), ObjectId(456));
    ASSERT_EQ(EngineReturnStatus::INVALID_TRACK, status);

    std::tie(status, id) = _module_under_test->load_plugin("sushi.testing.passthrough",
                                                           "",
                                                           "",
                                                           PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);

    std::tie(status, id) = _module_under_test->load_plugin("not_found",
                                                           "dummyname",
                                                           "",
                                                           PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN_UID, status);

    std::tie(status, id) = _module_under_test->load_plugin("",
                                                           "dummyname",
                                                           "not_found",
                                                           PluginType::VST2X);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN_UID, status);

    status = _module_under_test->remove_plugin_from_track(ObjectId(345), left_track_id);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);
}

TEST_F(TestEngine, TestSetSamplerate)
{
    auto [track_status, track_id] = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);
    auto [load_status, id] = _module_under_test->load_plugin("sushi.testing.equalizer",
                                                             "eq",
                                                             "",
                                                             PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, load_status);
    auto status = _module_under_test->add_plugin_to_track(id, _processors->track("left")->id());
    ASSERT_GE(EngineReturnStatus::OK, status);

    _module_under_test->set_sample_rate(48000.0f);
    ASSERT_FLOAT_EQ(48000.0f, _module_under_test->sample_rate());
    /* Pretty ugly way of checking that it was actually set, but wth */
    auto eq_plugin = static_cast<const equalizer_plugin::EqualizerPlugin*>(_module_under_test->_processors.processor("eq").get());
    ASSERT_FLOAT_EQ(48000.0f, eq_plugin->_sample_rate);
}

TEST_F(TestEngine, TestRealtimeConfiguration)

{
    auto faux_rt_thread = [](AudioEngine* e)
    {
        SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(2);
        SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(2);
        ControlBuffer control_buffer;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        e->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);
    };
    // Add a track, then a plugin to it while the engine is running, i.e. do it by asynchronous events instead
    _module_under_test->enable_realtime(true);
    auto rt = std::thread(faux_rt_thread, _module_under_test.get());
    auto [track_status, track_id] = _module_under_test->create_track("main", 2);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    rt = std::thread(faux_rt_thread, _module_under_test.get());
    auto [load_status, plugin_id] = _module_under_test->load_plugin("sushi.testing.gain",
                                                                    "gain_0_r",
                                                                    "   ",
                                                                    PluginType::INTERNAL);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, load_status);

    rt = std::thread(faux_rt_thread, _module_under_test.get());
    auto status = _module_under_test->add_plugin_to_track(plugin_id, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ASSERT_EQ(1u, _module_under_test->_audio_graph._audio_graph[0][0]->_processors.size());

    // Remove the plugin and track.

    // Deleting the plugin before removing it from the track should return an error
    status = _module_under_test->delete_plugin(plugin_id);
    ASSERT_EQ(EngineReturnStatus::ERROR, status);

    rt = std::thread(faux_rt_thread, _module_under_test.get());
    status = _module_under_test->remove_plugin_from_track(plugin_id, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph._audio_graph[0][0]->_processors.size());

    rt = std::thread(faux_rt_thread, _module_under_test.get());
    status = _module_under_test->delete_plugin(plugin_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    rt = std::thread(faux_rt_thread, _module_under_test.get());
    status = _module_under_test->delete_track("main");
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph._audio_graph[0].size());

    // Assert that they were also deleted from the map of processors
    ASSERT_FALSE(_processors->processor_exists("main"));
    ASSERT_FALSE(_processors->processor_exists("gain_0_r"));
    ASSERT_FALSE(_processors->processor_exists(plugin_id));
    ASSERT_FALSE(_processors->processor_exists(track_id));
    ASSERT_FALSE(_module_under_test->_realtime_processors[track_id]);
    ASSERT_FALSE(_module_under_test->_realtime_processors[plugin_id]);
}

TEST_F(TestEngine, TestSetCvChannels)
{
    EXPECT_EQ(EngineReturnStatus::OK, _module_under_test->set_cv_input_channels(2));
    EXPECT_EQ(EngineReturnStatus::OK, _module_under_test->set_cv_output_channels(2));
    // Set too many or route to non-existing inputs/processors
    EXPECT_NE(EngineReturnStatus::OK, _module_under_test->set_cv_input_channels(20));
    EXPECT_NE(EngineReturnStatus::OK, _module_under_test->set_cv_output_channels(20));

    EXPECT_NE(EngineReturnStatus::OK, _module_under_test->connect_cv_to_parameter("proc", "param", 1));
    EXPECT_NE(EngineReturnStatus::OK, _module_under_test->connect_cv_from_parameter("proc", "param", 1));
}

TEST_F(TestEngine, TestCvRouting)
{
    /* Add a control plugin track and connect cv to its parameters */
    auto [track_status, track_id] = _module_under_test->create_track("lfo_track", 0);
    auto [status, id] = _module_under_test->load_plugin("sushi.testing.lfo",
                                                        "lfo",
                                                        "   ",
                                                        PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->add_plugin_to_track(id, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->set_cv_input_channels(2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _module_under_test->set_cv_output_channels(2);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->connect_cv_to_parameter("lfo", "freq", 1);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    // First try with a too high cv input id
    status = _module_under_test->connect_cv_from_parameter("lfo", "out", 10);
    ASSERT_NE(EngineReturnStatus::OK, status);
    status = _module_under_test->connect_cv_from_parameter("lfo", "out", 1);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);
    ControlBuffer in_controls;
    ControlBuffer out_controls;

    in_controls.cv_values[1] = 0.5;
    _module_under_test->process_chunk(&in_buffer, &out_buffer, &in_controls, &out_controls, Time(0), 0);
    _module_under_test->process_chunk(&in_buffer, &out_buffer, &in_controls, &out_controls, Time(0), 0);

    // We should have a non-zero value in this slot
    ASSERT_NE(0.0f, out_controls.cv_values[1]);
}
TEST_F(TestEngine, TestGateRouting)
{
    /* Build a cv/gate to midi to cv/gate chain and verify gate changes travel through it*/
    _module_under_test->create_track("cv", 0);
    auto [cv_ctrl_status, cv_ctrl_id] = _module_under_test->load_plugin("sushi.testing.cv_to_control",
                                                                        "cv_ctrl",
                                                                        "   ",
                                                                        PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, cv_ctrl_status);
    auto status = _module_under_test->add_plugin_to_track(cv_ctrl_id, _processors->track("cv")->id());
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto [ctrl_cv_id_status, ctrl_cv_id] = _module_under_test->load_plugin("sushi.testing.control_to_cv",
                                                                           "ctrl_cv",
                                                                           "   ",
                                                                           PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, ctrl_cv_id_status);
    status = _module_under_test->add_plugin_to_track(ctrl_cv_id, _processors->track("cv")->id());
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->set_cv_input_channels(2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _module_under_test->set_cv_output_channels(2);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->connect_gate_to_processor("cv_ctrl", 1, 0, 0);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->connect_gate_from_processor("ctrl_cv", 0, 0, 0);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ChunkSampleBuffer in_buffer(1);
    ChunkSampleBuffer out_buffer(1);
    ControlBuffer in_controls;
    ControlBuffer out_controls;
    in_controls.gate_values.reset();
    in_controls.gate_values[1] = true;

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &in_controls, &out_controls, Time(0), 0);
    // A gate high event on gate input 1 should result in a gate high on gate output 0
    ASSERT_TRUE(out_controls.gate_values[0]);
    ASSERT_EQ(1u, out_controls.gate_values.count());
}