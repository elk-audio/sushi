#include <thread>

#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "engine/transport.h"

#define private public
#define protected public

#include "plugins/equalizer_plugin.h"
#include "engine/audio_engine.cpp"
#include "library/internal_processor_factory.cpp"
#include "library/plugin_registry.cpp"
#include "test_utils/dummy_processor.h"

constexpr float SAMPLE_RATE = 44000;
constexpr int TEST_CHANNEL_COUNT = 4;
using namespace sushi;
using namespace sushi::engine;

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

    /* But calling with audio_input set to true should trigger 2 new */
    _module_under_test.detect_clipped_samples(buffer, queue, true);
    ASSERT_FALSE(queue.empty());
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_EQ(ClipNotificationRtEvent::ClipChannelType::INPUT, notification.clip_notification_event()->channel_type());
    ASSERT_TRUE(queue.pop(notification));
    ASSERT_FALSE(queue.pop(notification));
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
    PluginInfo plugin_info;
    plugin_info.uid = "sushi.testing.gain";
    plugin_info.path = "";
    plugin_info.type = PluginType::INTERNAL;

    auto [load_status, plugin_id] = _module_under_test->create_processor(plugin_info, "gain");
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
    auto left_track_id = track_id;
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0].size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0][0]->name(),"left");

    /* Test invalid name */
    std::tie(status, track_id) = _module_under_test->create_track("left", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, track_id) = _module_under_test->create_track("", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);

    /* Test removal */
    status = _module_under_test->delete_track(left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_FALSE(_processors->processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph._audio_graph[0].size(),0u);

    /* Test invalid number of channels */
    std::tie(status, track_id) = _module_under_test->create_track("left", MAX_TRACK_CHANNELS + 1);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
}

TEST_F(TestEngine, TestCreatePreAndPostTracks)
{
    auto [status, track_id] = _module_under_test->create_pre_track("pre");
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto track = _processors->track("pre");
    ASSERT_TRUE(track);
    ASSERT_EQ(TrackType::PRE, track->type());
    ASSERT_EQ(track_id, track->id());

    std::tie(status, track_id) = _module_under_test->create_post_track("post");
    ASSERT_EQ(EngineReturnStatus::OK, status);

    track = _processors->track("post");
    ASSERT_TRUE(track);
    ASSERT_EQ(TrackType::POST, track->type());
    ASSERT_EQ(track_id, track->id());

    /* Test creating a second post track, this should fail */
    std::tie(status, track_id) = _module_under_test->create_post_track("post");
    ASSERT_NE(EngineReturnStatus::OK, status);
}

TEST_F(TestEngine, TestAddAndRemovePlugin)
{
    /* Test adding Internal plugins */
    auto [left_track_status, left_track_id] = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, left_track_status);

    PluginInfo gain_plugin_info;
    gain_plugin_info.uid = "sushi.testing.gain";
    gain_plugin_info.path = "";
    gain_plugin_info.type = PluginType::INTERNAL;

    auto [gain_status, gain_id] = _module_under_test->create_processor(gain_plugin_info, "gain");
    ASSERT_EQ(gain_status, EngineReturnStatus::OK);

    PluginInfo synth_plugin_info;
    synth_plugin_info.uid = "sushi.testing.sampleplayer";
    synth_plugin_info.path = "";
    synth_plugin_info.type = PluginType::INTERNAL;

    auto [synth_status, synth_id] = _module_under_test->create_processor(synth_plugin_info, "synth");
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

    PluginInfo plugin_info;
    plugin_info.uid = "sushi.testing.passthrough";
    plugin_info.path = "";
    plugin_info.type = PluginType::INTERNAL;

    std::tie(status, id) = _module_under_test->create_processor(plugin_info, "dummyname");

    status = _module_under_test->add_plugin_to_track(ObjectId(123), ObjectId(456));
    ASSERT_EQ(EngineReturnStatus::INVALID_TRACK, status);

    std::tie(status, id) = _module_under_test->create_processor(plugin_info, "");

    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);

    plugin_info.uid = "not_found";
    plugin_info.path = "";
    plugin_info.type = PluginType::INTERNAL;

    std::tie(status, id) = _module_under_test->create_processor(plugin_info, "");
    ASSERT_EQ(EngineReturnStatus::ERROR, status);

    plugin_info.uid = "not_found";
    plugin_info.path = "";
    plugin_info.type = PluginType::VST2X;
    std::tie(status, id) = _module_under_test->create_processor(plugin_info, "dummyname");
    ASSERT_NE(EngineReturnStatus::OK, status);

    status = _module_under_test->remove_plugin_from_track(ObjectId(345), left_track_id);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);
}

TEST_F(TestEngine, TestSetSamplerate)
{
    auto [track_status, track_id] = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    PluginInfo plugin_info;
    plugin_info.uid = "sushi.testing.equalizer";
    plugin_info.path = "";
    plugin_info.type = PluginType::INTERNAL;

    auto [load_status, id] = _module_under_test->create_processor(plugin_info, "eq");

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

    PluginInfo gain_plugin_info;
    gain_plugin_info.uid = "sushi.testing.gain";
    gain_plugin_info.path = "";
    gain_plugin_info.type = PluginType::INTERNAL;

    auto [load_status, plugin_id] = _module_under_test->create_processor(gain_plugin_info, "gain_0_r");
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
    status = _module_under_test->delete_track(track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->audio_input_connections().size());

    // Assert that they were also deleted from the map of processors
    ASSERT_FALSE(_processors->processor_exists("main"));
    ASSERT_FALSE(_processors->processor_exists("gain_0_r"));
    ASSERT_FALSE(_processors->processor_exists(plugin_id));
    ASSERT_FALSE(_processors->processor_exists(track_id));
    ASSERT_FALSE(_module_under_test->_realtime_processors[track_id]);
    ASSERT_FALSE(_module_under_test->_realtime_processors[plugin_id]);
}

TEST_F(TestEngine, TestAudioConnections)
{
    auto faux_rt_thread = [](AudioEngine* e, ChunkSampleBuffer* in, ChunkSampleBuffer* out, ControlBuffer* ctrl)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        e->process_chunk(in, out, ctrl, ctrl, Time(0), 0);
    };

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(4);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(4);
    ControlBuffer control_buffer;

    // Fill the channels with different values, so we can differentiate channels
    for (int i = 0; i < in_buffer.channel_count(); ++i)
    {
        auto channel_buffer = ChunkSampleBuffer::create_non_owning_buffer(in_buffer, i, 1);
        test_utils::fill_sample_buffer(channel_buffer, static_cast<float>(i + 1));
    }

    // Create a track and connect audio channels
    auto [track_status, track_id] = _module_under_test->create_track("main", 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);
    auto status = _module_under_test->connect_audio_input_channel(0, 0, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _module_under_test->connect_audio_output_channel(1, 0, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    EXPECT_EQ(1u, _module_under_test->audio_input_connections().size());
    EXPECT_EQ(1u, _module_under_test->audio_output_connections().size());

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);
    EXPECT_FLOAT_EQ(0.0, out_buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(1.0, out_buffer.channel(1)[0]);
    EXPECT_FLOAT_EQ(0.0, out_buffer.channel(2)[0]);
    EXPECT_FLOAT_EQ(0.0, out_buffer.channel(3)[0]);

    // Connect some while the engine is running
    _module_under_test->enable_realtime(true);
    auto rt = std::thread(faux_rt_thread, _module_under_test.get(), &in_buffer, &out_buffer, &control_buffer);
    status = _module_under_test->connect_audio_input_channel(3, 1, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    rt = std::thread(faux_rt_thread, _module_under_test.get(), &in_buffer, &out_buffer, &control_buffer);
    status = _module_under_test->connect_audio_output_channel(2, 1, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    EXPECT_FLOAT_EQ(0.0, out_buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(1.0, out_buffer.channel(1)[0]);
    EXPECT_FLOAT_EQ(4.0, out_buffer.channel(2)[0]);
    EXPECT_FLOAT_EQ(0.0, out_buffer.channel(3)[0]);

    // Remove the connections
    _module_under_test->enable_realtime(false);
    rt = std::thread(faux_rt_thread, _module_under_test.get(), &in_buffer, &out_buffer, &control_buffer);
    _module_under_test->_remove_connections_from_track(track_id);
    rt.join();
    EXPECT_EQ(0u, _module_under_test->audio_input_connections().size());
    EXPECT_EQ(0u, _module_under_test->audio_output_connections().size());
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

    PluginInfo lfo_plugin_info;
    lfo_plugin_info.uid = "sushi.testing.lfo";
    lfo_plugin_info.path = "";
    lfo_plugin_info.type = PluginType::INTERNAL;

    auto [status, id] = _module_under_test->create_processor(lfo_plugin_info, "lfo");
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

    PluginInfo cv_to_control_plugin_info;
    cv_to_control_plugin_info.uid = "sushi.testing.cv_to_control";
    cv_to_control_plugin_info.path = "";
    cv_to_control_plugin_info.type = PluginType::INTERNAL;
    auto [cv_ctrl_status, cv_ctrl_id] = _module_under_test->create_processor(cv_to_control_plugin_info, "cv_ctrl");

    ASSERT_EQ(EngineReturnStatus::OK, cv_ctrl_status);
    auto status = _module_under_test->add_plugin_to_track(cv_ctrl_id, _processors->track("cv")->id());
    ASSERT_EQ(EngineReturnStatus::OK, status);

    PluginInfo control_to_cv_plugin_info;
    control_to_cv_plugin_info.uid = "sushi.testing.control_to_cv";
    control_to_cv_plugin_info.path = "";
    control_to_cv_plugin_info.type = PluginType::INTERNAL;
    auto [ctrl_cv_id_status, ctrl_cv_id] = _module_under_test->create_processor(control_to_cv_plugin_info, "ctrl_cv");

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

TEST_F(TestEngine, TestMasterTrackProcessing)
{
    constexpr float GAIN_6DB = 126.0 / 144;

    ChunkSampleBuffer in_buffer(TEST_CHANNEL_COUNT);
    ChunkSampleBuffer out_buffer(TEST_CHANNEL_COUNT);
    ControlBuffer ctrl_buffer;
    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    auto [empty_status, empty_track_id] = _module_under_test->create_track("empty", TEST_CHANNEL_COUNT);
    ASSERT_EQ(EngineReturnStatus::OK, empty_status);

    auto [pre_status, pre_track_id] = _module_under_test->create_pre_track("pre");
    ASSERT_EQ(EngineReturnStatus::OK, pre_status);

    auto [post_status, post_track_id] = _module_under_test->create_post_track("post");
    ASSERT_EQ(EngineReturnStatus::OK, post_status);

    for (int i = 0; i < TEST_CHANNEL_COUNT; ++i)
    {
        _module_under_test->connect_audio_input_channel(i, i, empty_track_id);
        _module_under_test->connect_audio_output_channel(i, i, empty_track_id);
    }

    // Process and verify passthrough
    _module_under_test->process_chunk(&in_buffer, &out_buffer, &ctrl_buffer, &ctrl_buffer, Time(0), 0);
    test_utils::assert_buffer_value(1.0f, out_buffer);

    // Change the gain on the pre track and verify
    auto track = _processors->mutable_track("pre");
    auto gain_param = track->parameter_from_name("gain");
    auto gain_event = RtEvent::make_parameter_change_event(track->id(), 0, gain_param->id(), GAIN_6DB);

    track->process_event(gain_event);
    _module_under_test->process_chunk(&in_buffer, &out_buffer, &ctrl_buffer, &ctrl_buffer, Time(0), 0);
    EXPECT_GE(out_buffer.channel(0)[0], 1.0f);
}
