#include "gtest/gtest.h"

#define private public
#include "engine/controller/session_controller.cpp"
#undef private

#include "engine/audio_engine.h"
#include "control_frontends/base_control_frontend.h"
#include "test_utils/engine_mockup.h"
#include "test_utils/audio_frontend_mockup.h"
#include "plugins/equalizer_plugin.h"
#include "control_frontends/osc_frontend.h"
#include "test_utils/mock_osc_interface.h"
#include "test_utils/control_mockup.h"

using namespace sushi;
using namespace sushi::control_frontend;
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
        _audio_engine = std::make_unique<AudioEngine>(TEST_SAMPLE_RATE, 1, false, new EventDispatcherMockup());
        _mock_osc_interface = new MockOscInterface(0, 0, "");

        _osc_frontend = std::make_unique<OSCFrontend>(_audio_engine.get(), &_mock_controller, _mock_osc_interface);
        _event_dispatcher_mockup = static_cast<EventDispatcherMockup*>(_audio_engine->event_dispatcher());
        _midi_dispatcher = std::make_unique<MidiDispatcher>(_event_dispatcher_mockup);
        _module_under_test = std::make_unique<SessionController>(_audio_engine.get(), _midi_dispatcher.get(), &_audio_frontend);
        _module_under_test->set_osc_frontend(_osc_frontend.get());

        _audio_engine->set_audio_input_channels(8);
        _audio_engine->set_audio_output_channels(8);

    }

    void TearDown() {}
    MockOscInterface*                     _mock_osc_interface;
    sushi::ext::ControlMockup             _mock_controller;
    EventDispatcherMockup*                _event_dispatcher_mockup;

    AudioFrontendMockup                   _audio_frontend;
    std::unique_ptr<AudioEngine>          _audio_engine;
    std::unique_ptr<MidiDispatcher>       _midi_dispatcher;
    std::unique_ptr<SessionController>    _module_under_test;
    std::unique_ptr<OSCFrontend>          _osc_frontend;
};

TEST_F(SessionControllerTest, TestEmptyEngineState)
{
    auto state = _module_under_test->save_session();
    EXPECT_FALSE(state.save_date.empty());
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

    // used_audio_in/outputs should reflect the minimum channels needed to restore the session
    EXPECT_EQ(1, engine_state.used_audio_inputs);
    EXPECT_EQ(2, engine_state.used_audio_outputs);
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
    EXPECT_EQ(2, track.channels);
    EXPECT_EQ(1, track.buses);
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


TEST_F(SessionControllerTest, TestSaveAndRestore)
{
    std::string TRACK_NAME = "track_1";
    std::string PROCESSOR_NAME = "processor_1";
    constexpr int MIDI_PORT = 1;
    constexpr int PARAMETER_ID = 1;
    constexpr int CC_ID = 15;
    constexpr MidiChannel MIDI_CH = MidiChannel::CH_10;

    auto [track_status, track_id] = _audio_engine->create_track(TRACK_NAME, 2);
    ASSERT_EQ(EngineReturnStatus::OK, track_status);

    auto [status, proc_id] = _audio_engine->create_processor({.uid = std::string(equalizer_plugin::EqualizerPlugin::static_uid()),
                                                                     .path = "",
                                                                     .type = PluginType::INTERNAL},
                                                             PROCESSOR_NAME);
    ASSERT_EQ(EngineReturnStatus::OK, status);
    status = _audio_engine->add_plugin_to_track(proc_id, track_id);
    ASSERT_EQ(EngineReturnStatus::OK, status);

    // Make some midi connections
    _midi_dispatcher->set_midi_inputs(2);
    _midi_dispatcher->set_midi_outputs(1);
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_raw_midi_to_track(MIDI_PORT, track_id));
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_cc_to_parameter(MIDI_PORT, proc_id, PARAMETER_ID, CC_ID, 0, 1, false, MIDI_CH));
    ASSERT_EQ(MidiDispatcherStatus::OK, _midi_dispatcher->connect_pc_to_processor(MIDI_PORT, proc_id, MIDI_CH));

    // Set some engine parameters
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

    // Make some audio connections
    ASSERT_EQ(EngineReturnStatus::OK, _audio_engine->connect_audio_input_channel(1, 1, track_id));
    ASSERT_EQ(EngineReturnStatus::OK, _audio_engine->connect_audio_output_channel(2, 0, track_id));

    // Save the state, clear all track and reload the state
    auto session_state = _module_under_test->save_session();

    _module_under_test->_clear_all_tracks();
    _midi_dispatcher->disconnect_all_cc_from_processor(proc_id);
    _midi_dispatcher->disconnect_raw_midi_from_track(MIDI_PORT, track_id, MidiChannel::OMNI);
    _midi_dispatcher->disconnect_track_from_output(MIDI_PORT, track_id, MIDI_CH);

    ASSERT_TRUE(_audio_engine->processor_container()->all_tracks().empty());

    auto controller_status = _module_under_test->restore_session(session_state);
    // As the restore is done asynchronously, we need to execute the event manually

    _event_dispatcher_mockup->execute_engine_event(_audio_engine.get());

    // Check that tracks and processors were restored correctly
    auto processors = _audio_engine->processor_container();
    ASSERT_EQ(ext::ControlStatus::OK, controller_status);
    ASSERT_EQ(1, processors->all_tracks().size());
    auto restored_track = processors->all_tracks().front();

    ASSERT_EQ(1, processors->processors_on_track(restored_track->id()).size());
    auto restored_plugin = processors->processors_on_track(restored_track->id()).front();

    EXPECT_EQ(session_state.tracks.front().name, restored_track->name());
    EXPECT_EQ(session_state.tracks.front().label, restored_track->label());
    EXPECT_EQ(session_state.tracks.front().channels, restored_track->input_channels());
    EXPECT_EQ(session_state.tracks.front().channels, restored_track->output_channels());
    EXPECT_EQ(session_state.tracks.front().buses, restored_track->buses());
    EXPECT_EQ(session_state.tracks.front().track_state.parameters[0].second, restored_track->parameter_value(0).second);

    EXPECT_EQ(session_state.tracks.front().processors.front().name, restored_plugin->name());
    EXPECT_EQ(session_state.tracks.front().processors.front().label, restored_plugin->label());
    EXPECT_EQ(session_state.tracks.front().processors.front().state.parameters[0].second, restored_plugin->parameter_value(0).second);

    // Check that engine was restored correctly
    ASSERT_EQ(1, _audio_engine->audio_input_connections().size());
    ASSERT_EQ(1, _audio_engine->audio_output_connections().size());
    auto connection = _audio_engine->audio_input_connections().front();
    EXPECT_EQ(1, connection.engine_channel);
    EXPECT_EQ(1, connection.track_channel);
    EXPECT_EQ(restored_track->id(), connection.track);
    connection = _audio_engine->audio_output_connections().front();
    EXPECT_EQ(2, connection.engine_channel);
    EXPECT_EQ(0, connection.track_channel);
    EXPECT_EQ(restored_track->id(), connection.track);

    EXPECT_EQ(TEST_SAMPLE_RATE, _audio_engine->sample_rate());
    EXPECT_EQ(125, _audio_engine->transport()->current_tempo());
    EXPECT_EQ(SyncMode::MIDI, _audio_engine->transport()->sync_mode());
    EXPECT_EQ(PlayingMode::STOPPED, _audio_engine->transport()->playing_mode());
    EXPECT_EQ(TimeSignature({6, 8}), _audio_engine->transport()->time_signature());

    EXPECT_TRUE(_audio_engine->master_limiter());
    EXPECT_TRUE(_audio_engine->input_clip_detection());
    EXPECT_FALSE(_audio_engine->output_clip_detection());

    // Check midi connections
    auto kbd_in_routes = _midi_dispatcher->get_all_kb_input_connections();
    ASSERT_EQ(1u, kbd_in_routes.size());
    EXPECT_EQ(restored_track->id(), kbd_in_routes.front().input_connection.target);
    EXPECT_TRUE(kbd_in_routes.front().raw_midi);
    EXPECT_EQ(MIDI_PORT, kbd_in_routes.front().port);
    EXPECT_EQ(MidiChannel::OMNI, kbd_in_routes.front().channel);

    ASSERT_EQ(0u, _midi_dispatcher->get_all_kb_output_connections().size());

    auto cc_routes = _midi_dispatcher->get_cc_input_connections_for_processor(restored_plugin->id());
    ASSERT_EQ(1u, cc_routes.size());
    EXPECT_EQ(MIDI_PORT, cc_routes.front().port);
    EXPECT_EQ(CC_ID, cc_routes.front().cc);
    EXPECT_EQ(MIDI_CH, cc_routes.front().channel);

    auto pc_routes = _midi_dispatcher->get_pc_input_connections_for_processor(restored_plugin->id());
    ASSERT_EQ(1u, pc_routes.size());
    EXPECT_EQ(MIDI_PORT, pc_routes.front().port);
    EXPECT_EQ(MIDI_CH, pc_routes.front().channel);
}