#include <fstream>

#include "gtest/gtest.h"

#define private public
#define protected public

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>

#include "engine/json_configurator.cpp"

#undef private
#undef protected

#include "engine/audio_engine.h"
#include "engine/midi_dispatcher.h"
#include "test_utils/test_utils.h"
#include "test_utils/control_mockup.h"

#include "test_utils/mock_osc_interface.h"

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::_;

constexpr unsigned int SAMPLE_RATE = 44000;
constexpr unsigned int ENGINE_CHANNELS = 8;

using namespace sushi;
using namespace sushi::engine;
using namespace sushi::jsonconfig;
using namespace sushi::control_frontend;

class TestJsonConfigurator : public ::testing::Test
{
protected:
    TestJsonConfigurator() {}

    void SetUp()
    {
        _engine.set_audio_input_channels(ENGINE_CHANNELS);
        _engine.set_audio_output_channels(ENGINE_CHANNELS);
        _path = test_utils::get_data_dir_path();
        _path.append("config.json");
        _module_under_test = std::make_unique<JsonConfigurator>(&_engine,
                                                                &_midi_dispatcher,
                                                                _engine.processor_container(),
                                                                _path);
    }

    void TearDown()
    {
    }

    /* Helper functions */
    JsonConfigReturnStatus _make_track(const rapidjson::Value &track, TrackType type);

    AudioEngine _engine{SAMPLE_RATE};
    MidiDispatcher _midi_dispatcher{_engine.event_dispatcher()};

    sushi::ext::ControlMockup _controller;

    std::unique_ptr<JsonConfigurator> _module_under_test;

    std::string _path;
};

JsonConfigReturnStatus TestJsonConfigurator::_make_track(const rapidjson::Value &track, TrackType type)
{
    return _module_under_test->_make_track(track, type);
}

TEST_F(TestJsonConfigurator, TestLoadAudioConfig)
{
    auto [status, audio_config] = _module_under_test->load_audio_config();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_TRUE(audio_config.cv_inputs.has_value());
    ASSERT_EQ(1, audio_config.cv_inputs.value());
    ASSERT_TRUE(audio_config.cv_outputs.has_value());
    ASSERT_EQ(2, audio_config.cv_outputs.value());
}

TEST_F(TestJsonConfigurator, TestLoadHostConfig)
{
    auto status = _module_under_test->load_host_config();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_FLOAT_EQ(48000.0f, _engine.sample_rate());
}

TEST_F(TestJsonConfigurator, TestLoadTracks)
{
    auto status = _module_under_test->load_tracks();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    auto tracks = _engine.processor_container()->all_tracks();

    ASSERT_EQ(5u, tracks.size());
    auto track_1_processors = _engine.processor_container()->processors_on_track(tracks[0]->id());
    auto track_2_processors = _engine.processor_container()->processors_on_track(tracks[1]->id());

    ASSERT_EQ(3u, track_1_processors.size());
    ASSERT_EQ(3u, track_2_processors.size());

    ASSERT_EQ("passthrough_0_l", track_1_processors[0]->name());
    ASSERT_EQ("gain_0_l", track_1_processors[1]->name());
    ASSERT_EQ("equalizer_0_l", track_1_processors[2]->name());

    ASSERT_EQ("gain_0_r", track_2_processors[0]->name());
    ASSERT_EQ("passthrough_0_r", track_2_processors[1]->name());
    ASSERT_EQ("gain_1_r", track_2_processors[2]->name());
}

TEST_F(TestJsonConfigurator, TestLoadMidi)
{
    auto status = _module_under_test->load_tracks();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    _midi_dispatcher.set_midi_inputs(1);
    _midi_dispatcher.set_midi_outputs(1);

    status = _module_under_test->load_midi();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_EQ(1u, _midi_dispatcher._kb_routes_in.size());
    ASSERT_EQ(1u, _midi_dispatcher._cc_routes.size());
    ASSERT_EQ(1u, _midi_dispatcher._raw_routes_in.size());
    ASSERT_EQ(1u, _midi_dispatcher._pc_routes.size());
    ASSERT_TRUE(_midi_dispatcher.midi_clock_enabled(0));
}

TEST_F(TestJsonConfigurator, TestLoadOsc)
{
    // osc_frontend is only used in this test, so no need to keep in harness.
    constexpr int OSC_TEST_SERVER_PORT = 24024;
    constexpr int OSC_TEST_SEND_PORT = 24023;
    constexpr auto OSC_TEST_SEND_ADDRESS = "127.0.0.1";

    auto osc_interface = new NiceMock<MockOscInterface>(OSC_TEST_SERVER_PORT,
                                                        OSC_TEST_SEND_PORT,
                                                        OSC_TEST_SEND_ADDRESS);

    OSCFrontend osc_frontend{&_engine, &_controller, osc_interface};

    _module_under_test->set_osc_frontend(&osc_frontend);

    EXPECT_CALL(*osc_interface, init()).Times(1).WillOnce(Return(true));

    ASSERT_EQ(ControlFrontendStatus::OK, osc_frontend.init());

    auto status = _module_under_test->load_tracks();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    auto outputs_before = osc_frontend.get_enabled_parameter_outputs();

    ASSERT_EQ(0u, outputs_before.size());

    status = _module_under_test->load_osc();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    auto outputs_after = osc_frontend.get_enabled_parameter_outputs();

    ASSERT_EQ(1u, outputs_after.size());
}

TEST_F(TestJsonConfigurator, TestLoadCvGateControl)
{
    auto status = _module_under_test->load_tracks();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    status = _module_under_test->load_cv_gate();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
}

TEST_F(TestJsonConfigurator, TestLoadInitialState)
{
    auto status = _module_under_test->load_tracks();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    status = _module_under_test->load_initial_state();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    auto main_instance = _engine.processor_container()->mutable_processor("main");
    ASSERT_TRUE(main_instance.get());
    auto pan_info = main_instance->parameter_from_name("pan");
    ASSERT_TRUE(pan_info);
    ASSERT_FLOAT_EQ(0.35, main_instance->parameter_value(pan_info->id()).second);
}

TEST_F(TestJsonConfigurator, TestMakeChain)
{
    /* Create plugin track without processors */
    rapidjson::Document test_cfg;
    rapidjson::Value track(rapidjson::kObjectType);
    rapidjson::Value channels(1);
    rapidjson::Value name("track_without_plugins");
    rapidjson::Value inputs(rapidjson::kArrayType);
    rapidjson::Value outputs(rapidjson::kArrayType);
    rapidjson::Value plugins(rapidjson::kArrayType);
    track.AddMember("channels", channels, test_cfg.GetAllocator());
    track.AddMember("name", name, test_cfg.GetAllocator());
    track.AddMember("inputs", inputs, test_cfg.GetAllocator());
    track.AddMember("outputs", outputs, test_cfg.GetAllocator());
    track.AddMember("plugins", plugins, test_cfg.GetAllocator());
    ASSERT_EQ(_make_track(track, TrackType::REGULAR), JsonConfigReturnStatus::OK);

    /* Similar Plugin track but with same track id */
    track["channels"] = 2;
    ASSERT_EQ(_make_track(track, TrackType::REGULAR), JsonConfigReturnStatus::INVALID_TRACK_NAME);

    /* Create valid plugin track with valid plugin */
    track["name"] = "tracks_with_internal_plugin";
    rapidjson::Value test_plugin(rapidjson::kObjectType);
    rapidjson::Value uid("sushi.testing.gain");
    rapidjson::Value path("empty_path");
    rapidjson::Value type("internal");
    rapidjson::Value plugin_name("internal_plugin");
    test_plugin.AddMember("uid", uid, test_cfg.GetAllocator());
    test_plugin.AddMember("path", path, test_cfg.GetAllocator());
    test_plugin.AddMember("type", type, test_cfg.GetAllocator());
    test_plugin.AddMember("name", plugin_name, test_cfg.GetAllocator());
    track["plugins"].PushBack(test_plugin, test_cfg.GetAllocator());
    ASSERT_EQ(_make_track(track, TrackType::REGULAR), JsonConfigReturnStatus::OK);

    rapidjson::Value& plugin = track["plugins"][0];
    track["name"] = "track_invalid_internal";
    plugin["name"] = "invalid_internal_plugin";
    plugin["uid"] = "wrong_uid";
    plugin["type"] = "internal";
    ASSERT_EQ(_make_track(track, TrackType::REGULAR), JsonConfigReturnStatus::INVALID_CONFIGURATION);

    track["name"] = "track_invalid_name";
    plugin["name"] = "internal_plugin";
    plugin["uid"] = "sushi.testing.gain";
    plugin["type"] = "internal";
    ASSERT_EQ(_make_track(track, TrackType::REGULAR), JsonConfigReturnStatus::INVALID_CONFIGURATION);
}

TEST_F(TestJsonConfigurator, TestValidJsonSchema)
{
    std::ifstream config_file(_path);
    std::string config_file_contents((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());
    rapidjson::Document test_cfg;
    test_cfg.Parse(config_file_contents.c_str());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::HOST_CONFIG));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::TRACKS));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::MIDI));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::CV_GATE));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::EVENTS));
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg,JsonSection::STATE));
}

TEST_F(TestJsonConfigurator, TestHostConfigSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();
    /* No definition of host_config */
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));

    /* no definition of samplerate */
    rapidjson::Value host_config(rapidjson::kObjectType);
    test_cfg.AddMember("host_config", host_config, test_cfg.GetAllocator());
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));

    /* invalid type */
    rapidjson::Value samplerate(rapidjson::kObjectType);
    test_cfg["host_config"].AddMember("samplerate", samplerate, test_cfg.GetAllocator());
    test_cfg["host_config"]["samplerate"] = "44100";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::HOST_CONFIG));
}

TEST_F(TestJsonConfigurator, TestPluginChainSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();

    rapidjson::Value tracks(rapidjson::kArrayType);
    test_cfg.AddMember("tracks", tracks, test_cfg.GetAllocator());

    /* Plugin track without plugin list defined is not ok, empty list defined is ok */
    rapidjson::Value example_track(rapidjson::kObjectType);
    rapidjson::Value channels(1);
    rapidjson::Value name("track_name");
    rapidjson::Value inputs(rapidjson::kArrayType);
    rapidjson::Value outputs(rapidjson::kArrayType);
    rapidjson::Value plugins(rapidjson::kArrayType);
    example_track.AddMember("channels", channels, test_cfg.GetAllocator());
    example_track.AddMember("name", name, test_cfg.GetAllocator());
    example_track.AddMember("inputs", inputs, test_cfg.GetAllocator());
    example_track.AddMember("outputs", outputs, test_cfg.GetAllocator());
    test_cfg["tracks"].PushBack(example_track, test_cfg.GetAllocator());
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    test_cfg["tracks"][0].AddMember("plugins", plugins, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));

    /* incorrect mode */
    test_cfg["tracks"][0]["channels"] = -1;
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    test_cfg["tracks"][0]["channels"] = 2;
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
}

TEST_F(TestJsonConfigurator, TestPluginSchema)
{
    rapidjson::Document test_cfg;
    test_cfg.SetObject();
    rapidjson::Value tracks(rapidjson::kArrayType);
    test_cfg.AddMember("tracks", tracks, test_cfg.GetAllocator());

    rapidjson::Value example_track(rapidjson::kObjectType);
    rapidjson::Value track_name("track_name");
    rapidjson::Value channels(1);
    rapidjson::Value inputs(rapidjson::kArrayType);
    rapidjson::Value outputs(rapidjson::kArrayType);
    rapidjson::Value plugins(rapidjson::kArrayType);
    example_track.AddMember("name", track_name, test_cfg.GetAllocator());
    example_track.AddMember("channels", channels, test_cfg.GetAllocator());
    example_track.AddMember("inputs", inputs, test_cfg.GetAllocator());
    example_track.AddMember("outputs", outputs, test_cfg.GetAllocator());
    example_track.AddMember("plugins", plugins, test_cfg.GetAllocator());
    test_cfg["tracks"].PushBack(example_track, test_cfg.GetAllocator());

    rapidjson::Value example_plugin(rapidjson::kObjectType);
    rapidjson::Value plugin_name("plugin_name");
    rapidjson::Value path("plugin_path");
    rapidjson::Value uid("plugin_name");
    rapidjson::Value type("internal");
    example_plugin.AddMember("name", plugin_name, test_cfg.GetAllocator());
    example_plugin.AddMember("type", type, test_cfg.GetAllocator());
    test_cfg["tracks"][0]["plugins"].PushBack(example_plugin, test_cfg.GetAllocator());
    rapidjson::Value& plugin = test_cfg["tracks"][0]["plugins"][0];

    /* type = internal; requires uid */
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin.AddMember("uid", uid, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin["type"] = "vst3x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));

    /* type = vst2x; requires path */
    plugin["type"] = "vst2x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin.AddMember("path", path, test_cfg.GetAllocator());
    plugin.RemoveMember("uid");
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin["type"] = "vst3x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));

    /* type = vst3x; requires uid & path */
    rapidjson::Value vst3_uid("vst3_uid");
    plugin.AddMember("uid", vst3_uid, test_cfg.GetAllocator());
    ASSERT_TRUE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));

    /* type = LV2; requires name & uri */
    plugin["type"] = "lv2";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin.AddMember("uri", path, test_cfg.GetAllocator());
    plugin.RemoveMember("uid");
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
    plugin["type"] = "vst3x";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(test_cfg, JsonSection::TRACKS));
}

TEST_F(TestJsonConfigurator, TestMidiSchema)
{
    auto [status, midi_cfg] =_module_under_test->_parse_section(JsonSection::MIDI);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    rapidjson::Document mutable_cfg;
    mutable_cfg.SetObject();
    rapidjson::Value val(rapidjson::kObjectType);
    mutable_cfg.AddMember("midi", val, mutable_cfg.GetAllocator());
    mutable_cfg["midi"].CopyFrom(midi_cfg, mutable_cfg.GetAllocator());

    rapidjson::Value& track_connections = mutable_cfg["midi"]["track_connections"][0];
    ASSERT_TRUE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::MIDI));
    track_connections["channel"] = "invalid";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::MIDI));
    track_connections["channel"] = 16;
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::MIDI));
}

TEST_F(TestJsonConfigurator, TestCvGateSchema)
{
    auto [status, test_cfg] =_module_under_test->_parse_section(JsonSection::CV_GATE);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    rapidjson::Document mutable_cfg;
    mutable_cfg.SetObject();
    rapidjson::Value val(rapidjson::kObjectType);
    mutable_cfg.AddMember("cv_control", val, mutable_cfg.GetAllocator());
    mutable_cfg["cv_control"].CopyFrom(test_cfg, mutable_cfg.GetAllocator());

    rapidjson::Value& cv_in = mutable_cfg["cv_control"]["cv_inputs"][0];
    ASSERT_TRUE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::CV_GATE));
    cv_in["parameter"] = "";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::CV_GATE));
    cv_in["parameter"] = "pitch";
    cv_in["processor"] = "";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::CV_GATE));
    cv_in["processor"] = "synth";

    rapidjson::Value& gate_out = mutable_cfg["cv_control"]["gate_outputs"][0];
    gate_out["mode"] = "sync__";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::CV_GATE));
    gate_out["mode"] = "note_event";
    gate_out["channel"] = 1234;
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::CV_GATE));
}

TEST_F(TestJsonConfigurator, TestInititalStateSchema)
{
    auto [status, test_cfg] =_module_under_test->_parse_section(JsonSection::STATE);
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);

    rapidjson::Document mutable_cfg;
    mutable_cfg.SetObject();
    rapidjson::Value val(rapidjson::kObjectType);

    mutable_cfg.AddMember("initial_state", val, mutable_cfg.GetAllocator());
    mutable_cfg["initial_state"].CopyFrom(test_cfg, mutable_cfg.GetAllocator());

    auto& state_1 = mutable_cfg["initial_state"].GetArray()[0];
    ASSERT_TRUE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::STATE));
    state_1["parameters"]["pan"] = 1.5;
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::STATE));
    state_1["parameters"]["pan"] = "0.37";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::STATE));
    state_1["parameters"]["pan"] = 0.37;
    state_1["program"] = "string";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::STATE));
    state_1["program"] = 5;
    state_1["bypassed"] = "off";
    ASSERT_FALSE(_module_under_test->_validate_against_schema(mutable_cfg, JsonSection::STATE));
    state_1["bypassed"] = true;
    ASSERT_TRUE(_module_under_test->_validate_against_schema(mutable_cfg,JsonSection::STATE));
 }

TEST_F(TestJsonConfigurator, TestLoadEventList)
{
    // Load the tracks first so we can find the processors
    ASSERT_EQ(JsonConfigReturnStatus::OK, _module_under_test->load_tracks());

    auto [status, events] = _module_under_test->load_event_list();
    ASSERT_EQ(JsonConfigReturnStatus::OK, status);
    ASSERT_EQ(4u, events.size());
}

TEST(TestSchemaValidation, TestSchemaMetaValidation)
{
    // RapidJson only validates the schemas to be valid json, not that they
    // actually follow a valid schema.

    const char* meta_schema_char =
        #include "test_utils/meta_schema_v4.json"
        ;

    rapidjson::Document meta_schema;
    meta_schema.Parse(meta_schema_char);
    ASSERT_FALSE(meta_schema.HasParseError());
    rapidjson::SchemaDocument schema_document(meta_schema);
    rapidjson::SchemaValidator validator(schema_document);

    rapidjson::Document schema;
    schema.Parse(section_schema(JsonSection::HOST_CONFIG));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::TRACKS));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::MIDI));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::OSC));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::CV_GATE));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::EVENTS));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

    schema.Parse(section_schema(JsonSection::STATE));
    ASSERT_FALSE(schema.HasParseError());
    ASSERT_TRUE(schema.Accept(validator));

}