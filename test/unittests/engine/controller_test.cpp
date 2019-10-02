#include "gtest/gtest.h"

//#define private public
#include "engine/audio_engine.h"
#include "engine/json_configurator.h"
#include "engine/midi_dispatcher.h"
#include "engine/controller.cpp"
#include "test_utils/test_utils.h"
//#undef private

using namespace sushi;
using namespace sushi::engine;

constexpr unsigned int TEST_SAMPLE_RATE = 48000;
constexpr unsigned int ENGINE_CHANNELS = 8;
const std::string TEST_FILE = "config.json";

class ControllerTest : public ::testing::Test
{
protected:
    ControllerTest()
    {
    }
    void SetUp(const std::string& config_file)
    {
        _engine.set_audio_input_channels(ENGINE_CHANNELS);
        _engine.set_audio_output_channels(ENGINE_CHANNELS);
        std::string path = test_utils::get_data_dir_path();
        path.append(config_file);
        ASSERT_EQ(jsonconfig::JsonConfigReturnStatus::OK, _configurator.load_host_config(path));
        ASSERT_EQ(jsonconfig::JsonConfigReturnStatus::OK, _configurator.load_tracks(path));
        _dispatcher = _engine.event_dispatcher();
        _module_under_test = _engine.controller();
        ASSERT_TRUE(_module_under_test != nullptr);
    }

    void TearDown()
    {
    }

    AudioEngine _engine{TEST_SAMPLE_RATE};
    midi_dispatcher::MidiDispatcher _midi_dispatcher{&_engine};
    jsonconfig::JsonConfigurator _configurator{&_engine, &_midi_dispatcher};
    dispatcher::BaseEventDispatcher* _dispatcher;
    ext::SushiControl* _module_under_test;
};

TEST_F(ControllerTest, TestMainEngineControls)
{
    SetUp(TEST_FILE);

    EXPECT_FLOAT_EQ(TEST_SAMPLE_RATE, _module_under_test->get_samplerate());
    EXPECT_EQ(ext::PlayingMode::PLAYING, _module_under_test->get_playing_mode());
    EXPECT_EQ(ext::SyncMode::INTERNAL, _module_under_test->get_sync_mode());
    EXPECT_FLOAT_EQ(100.0f, _module_under_test->get_tempo());
    auto sig =  _module_under_test->get_time_signature();
    EXPECT_EQ(4, sig.numerator);
    EXPECT_EQ(4, sig.denominator);
    auto tracks = _module_under_test->get_tracks();
    EXPECT_EQ(3u, tracks.size());
    EXPECT_EQ("main", tracks[0].name);
    EXPECT_EQ("", tracks[0].label);
    EXPECT_EQ(2, tracks[0].input_channels);
    EXPECT_EQ(1, tracks[0].input_busses);
    EXPECT_EQ(2, tracks[0].output_channels);
    EXPECT_EQ(1, tracks[0].output_busses);
    EXPECT_EQ(3, tracks[0].processor_count);

    EXPECT_EQ("monotrack", tracks[1].name);
    EXPECT_EQ("", tracks[1].label);
    EXPECT_EQ(1, tracks[1].input_channels);
    EXPECT_EQ(1, tracks[1].input_busses);
    EXPECT_EQ(1, tracks[1].output_channels);
    EXPECT_EQ(1, tracks[1].output_busses);
    EXPECT_EQ(3, tracks[1].processor_count);

    EXPECT_EQ("multi", tracks[2].name);
    EXPECT_EQ("", tracks[2].label);
    EXPECT_EQ(4, tracks[2].input_channels);
    EXPECT_EQ(2, tracks[2].input_busses);
    EXPECT_EQ(4, tracks[2].output_channels);
    EXPECT_EQ(2, tracks[2].output_busses);
    EXPECT_EQ(0, tracks[2].processor_count);
}

TEST_F(ControllerTest, TestKeyboardControls)
{
    SetUp(TEST_FILE);

    /* No sanity checks on track ids is currently done, so these are just called to excercise the code */
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_note_on(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_note_off(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_note_aftertouch(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_pitch_bend(0, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_aftertouch(0, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, _module_under_test->send_modulation(0, 0, 1.0));
}

TEST_F(ControllerTest, TestTrackControls)
{
    SetUp(TEST_FILE);

    auto [not_found_status, id_unused] = _module_under_test->get_track_id("not_found");
    EXPECT_EQ(ext::ControlStatus::NOT_FOUND, not_found_status);
    auto [status, id] = _module_under_test->get_track_id("main");
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto [info_status, info] = _module_under_test->get_track_info(id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ("main", info.name);
    EXPECT_EQ("", info.label);
    EXPECT_EQ(2, info.input_channels);
    EXPECT_EQ(1, info.input_busses);
    EXPECT_EQ(2, info.output_channels);
    EXPECT_EQ(1, info.output_busses);
    EXPECT_EQ(3, info.processor_count);

    auto [proc_status, processors] = _module_under_test->get_track_processors(id);
    ASSERT_EQ(ext::ControlStatus::OK, proc_status);

    EXPECT_EQ(3u, processors.size());
    EXPECT_EQ("passthrough_0_l", processors[0].name);
    EXPECT_EQ("Passthrough", processors[0].label);
    EXPECT_EQ(0, processors[0].program_count);
    EXPECT_EQ(0, processors[0].parameter_count);

    EXPECT_EQ("gain_0_l", processors[1].name);
    EXPECT_EQ("Gain", processors[1].label);
    EXPECT_EQ(0, processors[1].program_count);
    EXPECT_EQ(1, processors[1].parameter_count);

    EXPECT_EQ("equalizer_0_l", processors[2].name);
    EXPECT_EQ("Equalizer", processors[2].label);
    EXPECT_EQ(0, processors[2].program_count);
    EXPECT_EQ(3, processors[2].parameter_count);

    auto [param_status, parameters] = _module_under_test->get_track_parameters(id);
    ASSERT_EQ(ext::ControlStatus::OK, param_status);

    EXPECT_EQ(2u, parameters.size());
    EXPECT_EQ("gain", parameters[0].name);
    EXPECT_EQ("Gain", parameters[0].label);
    EXPECT_EQ("", parameters[0].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[0].type);
    EXPECT_TRUE(parameters[0].automatable);
    EXPECT_FLOAT_EQ(-120.0f, parameters[0].min_range);
    EXPECT_FLOAT_EQ(24.0f, parameters[0].max_range);

    EXPECT_EQ("pan", parameters[1].name);
    EXPECT_EQ("Pan", parameters[1].label);
    EXPECT_EQ("", parameters[1].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[1].type);
    EXPECT_TRUE(parameters[1].automatable);
    EXPECT_FLOAT_EQ(-1.0f, parameters[1].min_range);
    EXPECT_FLOAT_EQ(1.0f, parameters[1].max_range);

    DECLARE_UNUSED(id_unused);
}


TEST_F(ControllerTest, TestProcessorControls)
{
    SetUp(TEST_FILE);

    auto [not_found_status, id_unused] = _module_under_test->get_processor_id("not_found");
    EXPECT_EQ(ext::ControlStatus::NOT_FOUND, not_found_status);
    auto [status, id] = _module_under_test->get_processor_id("equalizer_0_l");
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto [info_status, info] = _module_under_test->get_processor_info(id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ(info.name, "equalizer_0_l");
    EXPECT_EQ(info.label, "Equalizer");
    EXPECT_EQ(info.program_count, 0);
    EXPECT_EQ(info.parameter_count, 3);

    auto [bypass_status, bypassed] = _module_under_test->get_processor_bypass_state(id);
    ASSERT_EQ(ext::ControlStatus::OK, bypass_status);
    ASSERT_FALSE(bypassed);

    auto [programs_status, prog_unused] = _module_under_test->get_processor_current_program(id);
    ASSERT_EQ(ext::ControlStatus::UNSUPPORTED_OPERATION, programs_status);

    auto [param_status, parameters] = _module_under_test->get_processor_parameters(id);
    ASSERT_EQ(ext::ControlStatus::OK, param_status);

    EXPECT_EQ(3u, parameters.size());
    EXPECT_EQ("frequency", parameters[0].name);
    EXPECT_EQ("Frequency", parameters[0].label);
    EXPECT_EQ("", parameters[0].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[0].type);
    EXPECT_TRUE(parameters[0].automatable);
    EXPECT_FLOAT_EQ(20.0f, parameters[0].min_range);
    EXPECT_FLOAT_EQ(20000.0f, parameters[0].max_range);

    EXPECT_EQ("gain", parameters[1].name);
    EXPECT_EQ("Gain", parameters[1].label);
    EXPECT_EQ("", parameters[1].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[1].type);
    EXPECT_TRUE(parameters[1].automatable);
    EXPECT_FLOAT_EQ(-24.0f, parameters[1].min_range);
    EXPECT_FLOAT_EQ(24.0f, parameters[1].max_range);

    EXPECT_EQ("q", parameters[2].name);
    EXPECT_EQ("Q", parameters[2].label);
    EXPECT_EQ("", parameters[2].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[2].type);
    EXPECT_TRUE(parameters[2].automatable);
    EXPECT_FLOAT_EQ(0.0f, parameters[2].min_range);
    EXPECT_FLOAT_EQ(10.0f, parameters[2].max_range);

    DECLARE_UNUSED(id_unused);
    DECLARE_UNUSED(prog_unused);
}


TEST_F(ControllerTest, TestParameterControls)
{
    SetUp(TEST_FILE);

    auto [status, proc_id] = _module_under_test->get_processor_id("equalizer_0_l");
    ASSERT_EQ(ext::ControlStatus::OK, status);
    auto [found_status, id] = _module_under_test->get_parameter_id(proc_id, "frequency");
    ASSERT_EQ(ext::ControlStatus::OK, found_status);

    auto [info_status, info] = _module_under_test->get_parameter_info(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ("frequency", info.name);
    EXPECT_EQ("Frequency", info.label);
    EXPECT_EQ("", info.unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, info.type);
    EXPECT_EQ(true, info.automatable);
    EXPECT_FLOAT_EQ(20.0f, info.min_range);
    EXPECT_FLOAT_EQ(20000.0f, info.max_range);


    auto [value_status, value] = _module_under_test->get_parameter_value(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, value_status);
    EXPECT_FLOAT_EQ(1000.0f, value);

    auto [norm_value_status,norm_value] = _module_under_test->get_parameter_value_normalised(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, norm_value_status);
    EXPECT_GE(norm_value, 0.0f);
    EXPECT_LE(norm_value, 1.0f);

    auto [str_value_status, str_value] = _module_under_test->get_parameter_value_as_string(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, str_value_status);
    EXPECT_EQ("1000.000000", str_value);
}