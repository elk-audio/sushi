#include "gtest/gtest.h"

#define private public
#include "engine/controller/session_controller.cpp"
#undef private

#include "engine/audio_engine.h"
#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "plugins/equalizer_plugin.h"
#include "control_frontends/osc_frontend.h"

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

    }

    void TearDown() {}

    EventDispatcherMockup*                _event_dispatcher_mockup{nullptr};
    std::unique_ptr<AudioEngine>          _audio_engine;
    std::unique_ptr<MidiDispatcher>       _midi_dispatcher;
    std::unique_ptr<SessionController>    _module_under_test;
};

TEST_F(SessionControllerTest, TestEmptyEngineState)
{
    auto state = _module_under_test->save_session();
    EXPECT_EQ(0u, state.tracks.size());
}

TEST_F(SessionControllerTest, TestSaveSushiInfo)
{
    auto info = _module_under_test->_save_build_info();
    EXPECT_NE("", info.build_date);
    EXPECT_NE("", info.version);
    EXPECT_NE("", info.commit_hash);
    EXPECT_GT(info.build_options.size(), 0u);
    EXPECT_EQ(AUDIO_CHUNK_SIZE, info.audio_buffer_size);
}

TEST_F(SessionControllerTest, TestSaveOscState)
{
    // TODO : Test when OscFrontend can be suitably mocked
}

TEST_F(SessionControllerTest, TestSaveMidiState)
{
    std::string TRACK_NAME = "track_1";
    std::string PROCESSOR_NAME = "processor_1";
    constexpr int MIDI_PORT = 1;
    constexpr int PARAMETER_ID = 1;
    constexpr int CC_ID = 15;
    constexpr MidiChannel MIDI_CH = MidiChannel::CH_10;
    constexpr ext::MidiChannel EXT_MIDI_CH = ext::MidiChannel::MIDI_CH_10;

    auto [track_status, track_id] = _audio_engine->create_track(TRACK_NAME, 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    auto [status, proc_id] = _audio_engine->create_processor({.uid = std::string(equalizer_plugin::EqualizerPlugin::static_uid()),
                                                                     .path = "",
                                                                     .type = PluginType::INTERNAL},
                                                             PROCESSOR_NAME);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    // Make some midi connections
    _midi_dispatcher->set_midi_inputs(2);
    _midi_dispatcher->set_midi_outputs(1);
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_raw_midi_to_track(MIDI_PORT, track_id));
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_cc_to_parameter(MIDI_PORT, proc_id, PARAMETER_ID, CC_ID, 0, 1, false, MIDI_CH));
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_pc_to_processor(MIDI_PORT, proc_id, MIDI_CH));

    auto midi_state = _module_under_test->_save_midi_state();

    // Verify saved state
    EXPECT_EQ(2, midi_state.inputs);
    EXPECT_EQ(1, midi_state.outputs);
    ASSERT_EQ(1u, midi_state.kbd_input_connections.size());
    ASSERT_EQ(0u, midi_state.kbd_output_connections.size());
    ASSERT_EQ(1u, midi_state.cc_connections.size());
    ASSERT_EQ(1u, midi_state.pc_connections.size());

    auto kbd_con = midi_state.kbd_input_connections.front();
    EXPECT_TRUE(kbd_con.raw_midi);
    EXPECT_EQ(TRACK_NAME, kbd_con.track);
    EXPECT_EQ(MIDI_PORT, kbd_con.port);
    EXPECT_EQ(ext::MidiChannel::MIDI_CH_OMNI, kbd_con.channel);

    auto cc_con = midi_state.cc_connections.front();
    EXPECT_EQ(PROCESSOR_NAME, cc_con.processor);
    EXPECT_EQ(PARAMETER_ID, cc_con.parameter_id);
    EXPECT_EQ(MIDI_PORT, cc_con.port);
    EXPECT_EQ(EXT_MIDI_CH, cc_con.channel);
    EXPECT_EQ(CC_ID, cc_con.cc_number);
    EXPECT_EQ(0.0, cc_con.min_range);
    EXPECT_EQ(1.0, cc_con.max_range);
    EXPECT_FALSE(cc_con.relative_mode);

    auto pc_con = midi_state.pc_connections.front();
    EXPECT_EQ(PROCESSOR_NAME, pc_con.processor);
    EXPECT_EQ(MIDI_PORT, pc_con.port);
    EXPECT_EQ(EXT_MIDI_CH, pc_con.channel);
}


TEST_F(SessionControllerTest, TestSaveEngineState)
{
    std::string TRACK_NAME = "track_1";

    auto [track_status, track_id] = _audio_engine->create_track(TRACK_NAME, 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    _audio_engine->set_audio_input_channels(8);
    _audio_engine->set_audio_output_channels(6);
    _audio_engine->set_cv_input_channels(0);
    _audio_engine->set_cv_output_channels(2);
    _audio_engine->set_sample_rate(TEST_SAMPLE_RATE);
    _audio_engine->set_tempo(125);
    _audio_engine->set_tempo_sync_mode(SyncMode::MIDI);
    _audio_engine->set_transport_mode(PlayingMode::STOPPED);
    _audio_engine->set_time_signature({6, 8 });
    _audio_engine->enable_input_clip_detection(true);
    _audio_engine->enable_master_limiter(true);
    ASSERT_EQ(EngineReturnStatus::OK, _audio_engine->connect_audio_input_channel(1, 1, track_id));
    ASSERT_EQ(EngineReturnStatus::OK, _audio_engine->connect_audio_output_channel(2, 0, track_id));

    auto engine_state = _module_under_test->_save_engine_state();

    EXPECT_EQ(8, engine_state.audio_inputs);
    EXPECT_EQ(6, engine_state.audio_outputs);
    EXPECT_EQ(0, engine_state.cv_inputs);
    EXPECT_EQ(2, engine_state.cv_outputs);
    EXPECT_EQ(TEST_SAMPLE_RATE, engine_state.sample_rate);
    EXPECT_EQ(125, engine_state.tempo);
    EXPECT_EQ(ext::PlayingMode::STOPPED, engine_state.playing_mode);
    EXPECT_EQ(ext::SyncMode::MIDI, engine_state.sync_mode);
    EXPECT_EQ(6, engine_state.time_signature.numerator);
    EXPECT_EQ(8, engine_state.time_signature.denominator);
    EXPECT_TRUE(engine_state.input_clip_detection);
    EXPECT_FALSE(engine_state.output_clip_detection);
    EXPECT_TRUE(engine_state.master_limiter);
    EXPECT_EQ(1u, engine_state.input_connections.size());
    EXPECT_EQ(1u, engine_state.output_connections.size());

    auto in_con = engine_state.input_connections.front();
    EXPECT_EQ(TRACK_NAME, in_con.track);
    EXPECT_EQ(1, in_con.engine_channel);
    EXPECT_EQ(1, in_con.track_channel);

    auto out_con = engine_state.output_connections.front();
    EXPECT_EQ(TRACK_NAME, out_con.track);
    EXPECT_EQ(2, out_con.engine_channel);
    EXPECT_EQ(0, out_con.track_channel);
}

TEST_F(SessionControllerTest, TestSaveTracks)
{
    std::string TRACK_NAME = "track_1";
    std::string PROCESSOR_NAME = "processor_1";

    auto [track_status, track_id] = _audio_engine->create_track(TRACK_NAME, 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    auto [status, proc_id] = _audio_engine->create_processor({.uid = std::string(equalizer_plugin::EqualizerPlugin::static_uid()),
                                                                     .path = "",
                                                                     .type = PluginType::INTERNAL},
                                                             PROCESSOR_NAME);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    ASSERT_EQ(EngineReturnStatus::OK, _audio_engine->add_plugin_to_track(proc_id, track_id));

    auto tracks = _module_under_test->_save_tracks();

    ASSERT_EQ(1u, tracks.size());
    auto& track = tracks.front();

    EXPECT_EQ(TRACK_NAME, track.name);
    EXPECT_EQ("", track.label);
    EXPECT_EQ(2, track.input_channels);
    EXPECT_EQ(2, track.output_channels);
    EXPECT_EQ(1, track.input_busses);
    EXPECT_EQ(1, track.output_busses);
    // Track has 3 parameters, gain, pan and mute. This is tested more thoroughly in the track tests
    EXPECT_EQ(3u, track.track_state.parameters.size());

    ASSERT_EQ(1u, track.processors.size());
    auto& processor = track.processors.front();

    EXPECT_EQ(PROCESSOR_NAME, processor.name);
    EXPECT_EQ("Equalizer", processor.label);
    EXPECT_EQ("", processor.path);
    EXPECT_EQ(equalizer_plugin::EqualizerPlugin::static_uid(), processor.uid);
    EXPECT_EQ(ext::PluginType::INTERNAL, processor.type);
    EXPECT_EQ(3u, processor.state.parameters.size());
}