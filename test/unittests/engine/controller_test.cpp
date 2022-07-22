#include "gtest/gtest.h"

#include "engine/audio_engine.h"
#include "engine/json_configurator.h"
#include "engine/controller/controller.cpp"
#include "test_utils/test_utils.h"
#include "test_utils/audio_frontend_mockup.h"

/* Currently testing Controller as a complete class
 * eventually we might want to test the individual
 * controller interfaces separately */
#include "engine/controller/system_controller.cpp"
#include "engine/controller/transport_controller.cpp"
#include "engine/controller/timing_controller.cpp"
#include "engine/controller/keyboard_controller.cpp"
#include "engine/controller/parameter_controller.cpp"
#include "engine/controller/program_controller.cpp"
#include "engine/controller/cv_gate_controller.cpp"

using namespace sushi;
using namespace sushi::engine;

constexpr unsigned int TEST_SAMPLE_RATE = 48000;
constexpr unsigned int ENGINE_CHANNELS = 8;
const std::string TEST_FILE = "config.json";

class ControllerTest : public ::testing::Test
{
protected:
    ControllerTest() {}

    void SetUp()
    {
        _engine.set_audio_input_channels(ENGINE_CHANNELS);
        _engine.set_audio_output_channels(ENGINE_CHANNELS);

        ASSERT_EQ(jsonconfig::JsonConfigReturnStatus::OK, _configurator.load_host_config());
        ASSERT_EQ(jsonconfig::JsonConfigReturnStatus::OK, _configurator.load_tracks());

        _module_under_test = std::make_unique<Controller>(&_engine, &_midi_dispatcher, &_audio_frontend);
        ChunkSampleBuffer buffer(8);
        ControlBuffer ctrl_buffer;
        // Run once so that pending changes are executed
        _engine.process_chunk(&buffer, &buffer, &ctrl_buffer, &ctrl_buffer, Time(0), 0);
        ASSERT_TRUE(_module_under_test != nullptr);
    }

    std::string _path{test_utils::get_data_dir_path() + TEST_FILE};
    AudioEngine _engine{TEST_SAMPLE_RATE};
    midi_dispatcher::MidiDispatcher _midi_dispatcher{_engine.event_dispatcher()};
    AudioFrontendMockup  _audio_frontend;
    jsonconfig::JsonConfigurator _configurator{&_engine, &_midi_dispatcher, _engine.processor_container(), _path};
    std::unique_ptr<ext::SushiControl> _module_under_test;
};

TEST_F(ControllerTest, TestMainEngineControls)
{
    auto transport_controller = _module_under_test->transport_controller();
    ASSERT_TRUE(transport_controller);
    EXPECT_FLOAT_EQ(TEST_SAMPLE_RATE, transport_controller->get_samplerate());
    EXPECT_EQ(ext::PlayingMode::PLAYING, transport_controller->get_playing_mode());
    EXPECT_EQ(ext::SyncMode::INTERNAL, transport_controller->get_sync_mode());
    EXPECT_FLOAT_EQ(100.0f, transport_controller->get_tempo());
    auto sig =  transport_controller->get_time_signature();
    EXPECT_EQ(4, sig.numerator);
    EXPECT_EQ(4, sig.denominator);

    auto graph_controller = _module_under_test->audio_graph_controller();
    ASSERT_TRUE(graph_controller);
    auto tracks = graph_controller->get_all_tracks();

    ASSERT_EQ(5u, tracks.size());
    EXPECT_EQ("main", tracks[0].name);
    EXPECT_EQ("", tracks[0].label);
    EXPECT_EQ(2, tracks[0].channels);
    EXPECT_EQ(1, tracks[0].buses);
    EXPECT_EQ(ext::TrackType::REGULAR, tracks[0].type);
    EXPECT_EQ(3u, tracks[0].processors.size());

    EXPECT_EQ("monotrack", tracks[1].name);
    EXPECT_EQ("", tracks[1].label);
    EXPECT_EQ(1, tracks[1].channels);
    EXPECT_EQ(1, tracks[1].buses);
    EXPECT_EQ(ext::TrackType::REGULAR, tracks[1].type);
    EXPECT_EQ(3u, tracks[1].processors.size());

    EXPECT_EQ("monobustrack", tracks[2].name);
    EXPECT_EQ("", tracks[2].label);
    EXPECT_EQ(1, tracks[2].channels);
    EXPECT_EQ(1, tracks[2].buses);
    EXPECT_EQ(ext::TrackType::REGULAR, tracks[2].type);
    EXPECT_EQ(0u, tracks[2].processors.size());

    EXPECT_EQ("multi", tracks[3].name);
    EXPECT_EQ("", tracks[3].label);
    EXPECT_EQ(4, tracks[3].channels);
    EXPECT_EQ(2, tracks[3].buses);
    EXPECT_EQ(ext::TrackType::REGULAR, tracks[3].type);
    EXPECT_EQ(0u, tracks[3].processors.size());

    EXPECT_EQ("master", tracks[4].name);
    EXPECT_EQ("", tracks[4].label);
    EXPECT_EQ(ENGINE_CHANNELS, tracks[4].channels);
    EXPECT_EQ(1, tracks[4].buses);
    EXPECT_EQ(ext::TrackType::POST, tracks[4].type);
    EXPECT_EQ(0u, tracks[4].processors.size());
}

TEST_F(ControllerTest, TestKeyboardControls)
{
    auto keyboard_controller = _module_under_test->keyboard_controller();
    ASSERT_TRUE(keyboard_controller);
    /* No sanity checks on track ids is currently done, so these are just called to excercise the code */
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_note_on(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_note_off(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_note_aftertouch(0, 40, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_pitch_bend(0, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_aftertouch(0, 0, 1.0));
    EXPECT_EQ(ext::ControlStatus::OK, keyboard_controller->send_modulation(0, 0, 1.0));
}

TEST_F(ControllerTest, TestTrackControls)
{
    auto graph_controller = _module_under_test->audio_graph_controller();
    ASSERT_TRUE(graph_controller);

    auto [not_found_status, id_unused] = graph_controller->get_track_id("not_found");
    EXPECT_EQ(ext::ControlStatus::NOT_FOUND, not_found_status);
    auto [status, id] = graph_controller->get_track_id("main");
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto [track_not_found_status, processor_list] = graph_controller->get_track_processors(ObjectId(1234));
    ASSERT_EQ(ext::ControlStatus::NOT_FOUND, track_not_found_status);

    auto [info_status, info] = graph_controller->get_track_info(id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ("main", info.name);
    EXPECT_EQ("", info.label);
    EXPECT_EQ(2, info.channels);
    EXPECT_EQ(1, info.buses);
    EXPECT_EQ(3u, info.processors.size());

    auto [proc_status, processors] = graph_controller->get_track_processors(id);
    ASSERT_EQ(ext::ControlStatus::OK, proc_status);

    EXPECT_EQ(3u, processors.size());
    EXPECT_EQ("passthrough_0_l", processors[0].name);
    EXPECT_EQ("Passthrough", processors[0].label);
    EXPECT_EQ(0, processors[0].program_count);
    EXPECT_EQ(0, processors[0].parameter_count);
    EXPECT_EQ(info.processors[0], processors[0].id);

    EXPECT_EQ("gain_0_l", processors[1].name);
    EXPECT_EQ("Gain", processors[1].label);
    EXPECT_EQ(0, processors[1].program_count);
    EXPECT_EQ(1, processors[1].parameter_count);
    EXPECT_EQ(info.processors[1], processors[1].id);

    EXPECT_EQ("equalizer_0_l", processors[2].name);
    EXPECT_EQ("Equalizer", processors[2].label);
    EXPECT_EQ(0, processors[2].program_count);
    EXPECT_EQ(3, processors[2].parameter_count);
    EXPECT_EQ(info.processors[2], processors[2].id);

    auto parameter_controller = _module_under_test->parameter_controller();
    ASSERT_TRUE(parameter_controller);
    auto [param_status, parameters] = parameter_controller->get_track_parameters(id);
    ASSERT_EQ(ext::ControlStatus::OK, param_status);

    EXPECT_EQ(3u, parameters.size());
    EXPECT_EQ("gain", parameters[0].name);
    EXPECT_EQ("Gain", parameters[0].label);
    EXPECT_EQ("dB", parameters[0].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[0].type);
    EXPECT_TRUE(parameters[0].automatable);
    EXPECT_FLOAT_EQ(-120.0f, parameters[0].min_domain_value);
    EXPECT_FLOAT_EQ(24.0f, parameters[0].max_domain_value);

    EXPECT_EQ("pan", parameters[1].name);
    EXPECT_EQ("Pan", parameters[1].label);
    EXPECT_EQ("", parameters[1].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[1].type);
    EXPECT_TRUE(parameters[1].automatable);
    EXPECT_FLOAT_EQ(-1.0f, parameters[1].min_domain_value);
    EXPECT_FLOAT_EQ(1.0f, parameters[1].max_domain_value);

    DECLARE_UNUSED(id_unused);
}

TEST_F(ControllerTest, TestProcessorControls)
{
    auto graph_controller = _module_under_test->audio_graph_controller();
    ASSERT_TRUE(graph_controller);

    auto [not_found_status, id_unused] = graph_controller->get_processor_id("not_found");
    EXPECT_EQ(ext::ControlStatus::NOT_FOUND, not_found_status);
    auto [status, id] = graph_controller->get_processor_id("equalizer_0_l");
    ASSERT_EQ(ext::ControlStatus::OK, status);

    auto [info_status, info] = graph_controller->get_processor_info(id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ(info.name, "equalizer_0_l");
    EXPECT_EQ(info.label, "Equalizer");
    EXPECT_EQ(info.program_count, 0);
    EXPECT_EQ(info.parameter_count, 3);

    auto [bypass_status, bypassed] = graph_controller->get_processor_bypass_state(id);
    ASSERT_EQ(ext::ControlStatus::OK, bypass_status);
    ASSERT_FALSE(bypassed);

    auto program_controller = _module_under_test->program_controller();
    ASSERT_TRUE(program_controller);
    auto [programs_status, prog_unused] = program_controller->get_processor_current_program(id);
    ASSERT_EQ(ext::ControlStatus::UNSUPPORTED_OPERATION, programs_status);

    auto parameter_controller = _module_under_test->parameter_controller();
    ASSERT_TRUE(parameter_controller);
    auto [param_status, parameters] = parameter_controller->get_processor_parameters(id);
    ASSERT_EQ(ext::ControlStatus::OK, param_status);

    EXPECT_EQ(3u, parameters.size());
    EXPECT_EQ("frequency", parameters[0].name);
    EXPECT_EQ("Frequency", parameters[0].label);
    EXPECT_EQ("Hz", parameters[0].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[0].type);
    EXPECT_TRUE(parameters[0].automatable);
    EXPECT_FLOAT_EQ(20.0f, parameters[0].min_domain_value);
    EXPECT_FLOAT_EQ(20000.0f, parameters[0].max_domain_value);

    EXPECT_EQ("gain", parameters[1].name);
    EXPECT_EQ("Gain", parameters[1].label);
    EXPECT_EQ("dB", parameters[1].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[1].type);
    EXPECT_TRUE(parameters[1].automatable);
    EXPECT_FLOAT_EQ(-24.0f, parameters[1].min_domain_value);
    EXPECT_FLOAT_EQ(24.0f, parameters[1].max_domain_value);

    EXPECT_EQ("q", parameters[2].name);
    EXPECT_EQ("Q", parameters[2].label);
    EXPECT_EQ("", parameters[2].unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, parameters[2].type);
    EXPECT_TRUE(parameters[2].automatable);
    EXPECT_FLOAT_EQ(0.0f, parameters[2].min_domain_value);
    EXPECT_FLOAT_EQ(10.0f, parameters[2].max_domain_value);

    DECLARE_UNUSED(id_unused);
    DECLARE_UNUSED(prog_unused);
}

TEST_F(ControllerTest, TestParameterControls)
{
    auto parameter_controller = _module_under_test->parameter_controller();
    ASSERT_TRUE(parameter_controller);
    auto graph_controller = _module_under_test->audio_graph_controller();
    ASSERT_TRUE(graph_controller);

    auto [status, proc_id] = graph_controller->get_processor_id("equalizer_0_l");
    ASSERT_EQ(ext::ControlStatus::OK, status);
    auto [found_status, id] = parameter_controller->get_parameter_id(proc_id, "frequency");
    ASSERT_EQ(ext::ControlStatus::OK, found_status);

    auto [info_status, info] = parameter_controller->get_parameter_info(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, info_status);

    EXPECT_EQ("frequency", info.name);
    EXPECT_EQ("Frequency", info.label);
    EXPECT_EQ("Hz", info.unit);
    EXPECT_EQ(ext::ParameterType::FLOAT, info.type);
    EXPECT_EQ(true, info.automatable);
    EXPECT_FLOAT_EQ(20.0f, info.min_domain_value);
    EXPECT_FLOAT_EQ(20000.0f, info.max_domain_value);

    auto [value_status, value] = parameter_controller->get_parameter_value_in_domain(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, value_status);
    EXPECT_FLOAT_EQ(1000.0f, value);

    auto [norm_value_status,norm_value] = parameter_controller->get_parameter_value(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, norm_value_status);
    EXPECT_GE(norm_value, 0.0f);
    EXPECT_LE(norm_value, 1.0f);

    auto [str_value_status, str_value] = parameter_controller->get_parameter_value_as_string(proc_id, id);
    ASSERT_EQ(ext::ControlStatus::OK, str_value_status);
    EXPECT_EQ("1000.00", str_value);
}
