#include <algorithm>
#include <thread>

#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "engine/transport.h"

#define private public
#define protected public

#include "engine/audio_engine.cpp"

constexpr unsigned int SAMPLE_RATE = 44000;
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

/*
* Engine tests
*/
class TestEngine : public ::testing::Test
{
protected:
    TestEngine() {}

    void SetUp()
    {
        _module_under_test = new AudioEngine(SAMPLE_RATE);
        _module_under_test->set_audio_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_audio_output_channels(TEST_CHANNEL_COUNT);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    AudioEngine* _module_under_test;
};

TEST_F(TestEngine, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

/*
 * Test that 1:s in gives 1:s out
 */
TEST_F(TestEngine, TestProcess)
{
    /* Add a plugin track and connect it to inputs and outputs */
    _module_under_test->create_track("test_track", 2);
    auto [create_status, track_id] = _module_under_test->processor_id_from_name("test_track");
    ASSERT_EQ(EngineReturnStatus::OK, create_status);

    auto res = _module_under_test->connect_audio_input_bus(0, 0, "test_track");
    ASSERT_EQ(EngineReturnStatus::OK, res);
    res = _module_under_test->connect_audio_output_bus(0, 0, "test_track");
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

    test_utils::assert_buffer_value(1.0f, main_bus);
    test_utils::assert_buffer_value(0.0f, second_bus);

    /* Add a plugin to the track and do the same thing */
    auto [load_status, plugin_id]  = _module_under_test->load_plugin("sushi.testing.gain", "gain", "", PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, load_status);
    res = _module_under_test->add_plugin_to_track(plugin_id, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, res);
    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);
    main_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);
    test_utils::assert_buffer_value(1.0f, main_bus);
}

TEST_F(TestEngine, TestOutputMixing)
{
    _module_under_test->create_track("1", 2);
    _module_under_test->create_track("2", 2);
    _module_under_test->connect_audio_input_bus(0, 0, "1");
    _module_under_test->connect_audio_input_bus(1, 0, "2");
    _module_under_test->connect_audio_output_bus(0, 0, "1");
    _module_under_test->connect_audio_output_bus(0, 0, "2");

    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(TEST_CHANNEL_COUNT);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(TEST_CHANNEL_COUNT);
    ControlBuffer control_buffer;
    test_utils::fill_sample_buffer(in_buffer, 1.0f);

    _module_under_test->process_chunk(&in_buffer, &out_buffer, &control_buffer, &control_buffer, Time(0), 0);

    /* Both track's outputs are routed to bus 0, so they should sum to 2 */
    auto main_bus = SampleBuffer<AUDIO_CHUNK_SIZE>::create_non_owning_buffer(out_buffer, 0, 2);

    test_utils::assert_buffer_value(2.0f, main_bus);
}


TEST_F(TestEngine, TestUidNameMapping)
{
    _module_under_test->create_track("left", 2);
    auto [track_status, track_id] = _module_under_test->processor_id_from_name(("left"));
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    auto [status, plugin_id] = _module_under_test->load_plugin("sushi.testing.equalizer",
                                                               "equalizer_0_l",
                                                               "",
                                                               PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->add_plugin_to_track(plugin_id, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ObjectId id;
    std::tie(status, id) = _module_under_test->processor_id_from_name("equalizer_0_l");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_TRUE(_module_under_test->_processor_exists(id));
    ASSERT_EQ(plugin_id, id);
    std::string name;
    std::tie(status, name) = _module_under_test->processor_name_from_id(id);
    ASSERT_TRUE(_module_under_test->_processor_exists(name));
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ("equalizer_0_l", name);

    /* Test with name/id that doesn't match any processors */
    std::tie(status, id) = _module_under_test->processor_id_from_name("not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, name) = _module_under_test->processor_name_from_id(123456);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);

    /* Test Parameter Lookup */
    std::tie(status, id) = _module_under_test->parameter_id_from_name("equalizer_0_l", "q");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    std::tie(status, id) = _module_under_test->parameter_id_from_name("not_found", "gain");
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, id) = _module_under_test->parameter_id_from_name("equalizer_0_l", "not_found");
    ASSERT_EQ(EngineReturnStatus::INVALID_PARAMETER, status);
    std::tie(status, name) = _module_under_test->parameter_name_from_id("equalizer_0_l", 1);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ("gain", name);
    std::tie(status, name) = _module_under_test->parameter_name_from_id("not_found", 10);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    std::tie(status, name) = _module_under_test->parameter_name_from_id("equalizer_0_l", 10);
    ASSERT_EQ(EngineReturnStatus::INVALID_PARAMETER, status);
}

TEST_F(TestEngine, TestCreateEmptyTrack)
{
    auto status = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_TRUE(_module_under_test->_processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph.size(),1u);
    ASSERT_EQ(_module_under_test->_audio_graph[0]->name(),"left");

    /* Test invalid name */
    status = _module_under_test->create_track("left", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PROCESSOR, status);
    status = _module_under_test->create_track("", 1);
    ASSERT_EQ(EngineReturnStatus::INVALID_PLUGIN, status);

    /* Test removal */
    status = _module_under_test->delete_track("left");
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_FALSE(_module_under_test->_processor_exists("left"));
    ASSERT_EQ(_module_under_test->_audio_graph.size(),0u);

    /* Test invalid number of channels */
    status = _module_under_test->create_track("left", 3);
    ASSERT_EQ(status, EngineReturnStatus::INVALID_N_CHANNELS);
}

TEST_F(TestEngine, TestAddAndRemovePlugin)
{
    /* Test adding Internal plugins */
    auto status = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
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

    auto left_track_id = _module_under_test->processor_id_from_name("left").second;
    status = _module_under_test->add_plugin_to_track(gain_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    /* Add synth before gain */
    status = _module_under_test->add_plugin_to_track(synth_id, left_track_id, gain_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    auto processors = _module_under_test->processors_on_track(left_track_id);
    ASSERT_EQ(2u, processors.size());
    ASSERT_EQ("synth", processors.front()->name());

    /* Check that processors exists and in the right order on track "main" */
    ASSERT_TRUE(_module_under_test->_processor_exists("gain"));
    ASSERT_TRUE(_module_under_test->_processor_exists("synth"));
    ASSERT_EQ(2u, _module_under_test->_audio_graph[0]->_processors.size());
    ASSERT_EQ("synth", _module_under_test->_audio_graph[0]->_processors[0]->name());
    ASSERT_EQ("gain", _module_under_test->_audio_graph[0]->_processors[1]->name());

    /* Move a processor from 1 track to another */
    status = _module_under_test->create_track("right", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    auto right_track_id = _module_under_test->processor_id_from_name("right").second;
    status = _module_under_test->remove_plugin_from_track(synth_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->add_plugin_to_track(synth_id, right_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto left_processors = _module_under_test->processors_on_track(left_track_id);
    auto right_processors = _module_under_test->processors_on_track(right_track_id);
    ASSERT_EQ(1u, left_processors.size());
    ASSERT_EQ("gain", left_processors.front()->name());
    ASSERT_EQ(1u, right_processors.size());
    ASSERT_EQ("synth", right_processors.front()->name());

    /* Test removing plugin */
    status = _module_under_test->remove_plugin_from_track(gain_id, left_track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    processors = _module_under_test->processors_on_track(left_track_id);
    ASSERT_EQ(0u, processors.size());

    status = _module_under_test->delete_plugin(gain_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ASSERT_FALSE(_module_under_test->_processor_exists("gain"));
    ASSERT_EQ(0u, _module_under_test->_audio_graph[0]->_processors.size());
    ASSERT_EQ("synth", _module_under_test->_audio_graph[1]->_processors[0]->name());

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
    auto status = _module_under_test->create_track("left", 2);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    auto [load_status, id] = _module_under_test->load_plugin("sushi.testing.equalizer",
                                                             "eq",
                                                             "",
                                                             PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, load_status);
    status = _module_under_test->add_plugin_to_track(id, _module_under_test->track("left")->id());
    ASSERT_GE(EngineReturnStatus::OK, status);

    _module_under_test->set_sample_rate(48000.0f);
    ASSERT_FLOAT_EQ(48000.0f, _module_under_test->sample_rate());
    /* Pretty ugly way of checking that it was actually set, but wth */
    auto eq_plugin = static_cast<equalizer_plugin::EqualizerPlugin*>(_module_under_test->_processors_by_name["eq"].get());
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
    auto rt = std::thread(faux_rt_thread, _module_under_test);
    auto status = _module_under_test->create_track("main", 2);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    rt = std::thread(faux_rt_thread, _module_under_test);
    auto [load_status, plugin_id] = _module_under_test->load_plugin("sushi.testing.gain",
                                                                    "gain_0_r",
                                                                    "   ",
                                                                    PluginType::INTERNAL);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, load_status);
    auto track_id = _module_under_test->track("main")->id();

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->add_plugin_to_track(plugin_id, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    ASSERT_EQ(1u, _module_under_test->_audio_graph[0]->_processors.size());

    // Remove the plugin and track.

    // Deleting the plugin before removing it from the track should return an error
    status = _module_under_test->delete_plugin(plugin_id);
    ASSERT_EQ(EngineReturnStatus::ERROR, status);

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->remove_plugin_from_track(plugin_id, track_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph[0]->_processors.size());

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->delete_plugin(plugin_id);
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);

    rt = std::thread(faux_rt_thread, _module_under_test);
    status = _module_under_test->delete_track("main");
    rt.join();
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(0u, _module_under_test->_audio_graph.size());

    // Assert that they were also deleted from the map of processors
    ASSERT_FALSE(_module_under_test->_processor_exists("main"));
    ASSERT_FALSE(_module_under_test->_processor_exists("gain_0_r"));
    ASSERT_FALSE(_module_under_test->_processor_exists(plugin_id));
    ASSERT_FALSE(_module_under_test->_processor_exists(track_id));
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
    _module_under_test->create_track("lfo_track", 0);
    auto [status, id] = _module_under_test->load_plugin("sushi.testing.lfo",
                                                        "lfo",
                                                        "   ",
                                                        PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    status = _module_under_test->add_plugin_to_track(id, _module_under_test->track("lfo_track")->id());
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
    auto status = _module_under_test->add_plugin_to_track(cv_ctrl_id, _module_under_test->track("cv")->id());
    ASSERT_EQ(EngineReturnStatus::OK, status);

    auto [ctrl_cv_id_status, ctrl_cv_id] = _module_under_test->load_plugin("sushi.testing.control_to_cv",
                                                                           "ctrl_cv",
                                                                           "   ",
                                                                           PluginType::INTERNAL);
    ASSERT_EQ(EngineReturnStatus::OK, ctrl_cv_id_status);
    status = _module_under_test->add_plugin_to_track(ctrl_cv_id, _module_under_test->track("cv")->id());
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