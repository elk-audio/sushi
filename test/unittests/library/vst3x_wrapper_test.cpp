#include <filesystem>

#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "library/rt_event_fifo.h"
#include "library/vst3x/vst3x_utils.cpp"
#include "test_utils/host_control_mockup.h"

#define private public
#include "library/vst3x/vst3x_wrapper.cpp"
#include "library/vst3x/vst3x_host_app.cpp"

using namespace sushi;
using namespace sushi::vst3;

#ifdef NDEBUG
const char PLUGIN_FILE[] = "../VST3/Release/adelay.vst3";
#else
const char PLUGIN_FILE[] = "../VST3/Debug/adelay.vst3";
#endif

const char PLUGIN_NAME[] = "ADelay";

constexpr unsigned int DELAY_PARAM_ID = 100;
constexpr unsigned int BYPASS_PARAM_ID = 101;
constexpr float TEST_SAMPLE_RATE = 48000;
constexpr int   TEST_CHANNEL_COUNT = 2;

/* Quick test to test plugin loading */
TEST(TestVst3xPluginInstance, TestLoadPlugin)
{
    auto full_path = std::filesystem::path(PLUGIN_FILE);
    auto full_test_plugin_path = std::string(std::filesystem::absolute(full_path));

    SushiHostApplication host_app;
    PluginInstance module_under_test(&host_app);
    bool success = module_under_test.load_plugin(full_test_plugin_path, PLUGIN_NAME);
    ASSERT_TRUE(success);
    ASSERT_TRUE(module_under_test.processor());
    ASSERT_TRUE(module_under_test.component());
    ASSERT_TRUE(module_under_test.controller());
}

/* Test that nothing breaks if the plugin is not found */
TEST(TestVst3xPluginInstance, TestLoadPluginFromErroneousFilename)
{
    /* Non-existing library */
    SushiHostApplication host_app;
    PluginInstance module_under_test(&host_app);
    bool success = module_under_test.load_plugin("/usr/lib/lxvst/no_plugin.vst3", PLUGIN_NAME);
    ASSERT_FALSE(success);

    /* Existing library but non-existing plugin */
    auto full_path = std::filesystem::path(PLUGIN_FILE);
    auto full_test_plugin_path = std::string(std::filesystem::absolute(full_path));

    success = module_under_test.load_plugin(full_test_plugin_path, "NoPluginWithThisName");
    ASSERT_FALSE(success);
}

TEST(TestVst3xRtState, TestOperation)
{
    ProcessorState state;
    state.add_parameter_change(3, 0.5f);
    state.add_parameter_change(10, 0.25f);
    Vst3xRtState module_under_test(state);

    EXPECT_EQ(2, module_under_test.getParameterCount());
    auto data = module_under_test.getParameterData(0);
    ASSERT_TRUE(data);
    EXPECT_EQ(1, data->getPointCount());
    EXPECT_EQ(3, data->getParameterId());
    Steinberg::Vst::ParamValue value = 0;
    Steinberg::int32 offset = -1;

    EXPECT_EQ(Steinberg::kResultOk, data->getPoint(0, offset, value));
    EXPECT_FLOAT_EQ(0.5, value);
    EXPECT_EQ(0, offset);

    data = module_under_test.getParameterData(1);
    ASSERT_TRUE(data);
    EXPECT_EQ(10, data->getParameterId());
    EXPECT_EQ(Steinberg::kResultOk, data->getPoint(0, offset, value));
    EXPECT_FLOAT_EQ(0.25, value);
    EXPECT_EQ(0, offset);

    data = module_under_test.getParameterData(2);
    EXPECT_FALSE(data);
}


class TestVst3xWrapper : public ::testing::Test
{
protected:
    using ::testing::Test::SetUp; // Hide error of hidden overload of virtual function in clang when signatures differ but the name is the same
    TestVst3xWrapper() {}

    void SetUp(const char* plugin_file, const char* plugin_name)
    {
        auto full_path = std::filesystem::path(plugin_file);
        auto full_plugin_path = std::string(std::filesystem::absolute(full_path));

        _module_under_test = std::make_unique<Vst3xWrapper>(_host_control.make_host_control_mockup(TEST_SAMPLE_RATE),
                                                            full_plugin_path,
                                                            plugin_name,
                                                            &_host_app);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
        _module_under_test->set_event_output(&_event_queue);
        _module_under_test->set_input_channels(TEST_CHANNEL_COUNT);
        _module_under_test->set_output_channels(TEST_CHANNEL_COUNT);
    }

    SushiHostApplication _host_app;
    HostControlMockup _host_control;
    std::unique_ptr<Vst3xWrapper> _module_under_test;
    RtSafeRtEventFifo _event_queue;
};

TEST_F(TestVst3xWrapper, TestLoadAndInitPlugin)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ASSERT_TRUE(_module_under_test.get());
    EXPECT_EQ("ADelay", _module_under_test->name());

    auto parameters = _module_under_test->all_parameters();
    EXPECT_EQ(1u, parameters.size());
    EXPECT_EQ("Delay", parameters[0]->name());
    EXPECT_EQ(DELAY_PARAM_ID, parameters[0]->id());
    EXPECT_TRUE(_module_under_test->_bypass_parameter.supported);
    EXPECT_EQ(BYPASS_PARAM_ID, static_cast<unsigned int>(_module_under_test->_bypass_parameter.id));

    auto descriptor = _module_under_test->parameter_from_name("Delay");
    ASSERT_TRUE(descriptor);
    EXPECT_EQ(DELAY_PARAM_ID, descriptor->id());

    descriptor = _module_under_test->parameter_from_id(DELAY_PARAM_ID);
    ASSERT_TRUE(descriptor);
    EXPECT_EQ(DELAY_PARAM_ID, descriptor->id());

    descriptor = _module_under_test->parameter_from_id(12345);
    ASSERT_FALSE(descriptor);
}

TEST_F(TestVst3xWrapper, TestProcessing)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1);
    /* Set delay to 0 */
    auto event = RtEvent::make_parameter_change_event(0u, 0, DELAY_PARAM_ID, 0.0f);

    _module_under_test->set_enabled(true);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);

    /* Minimum delay will still be 1 sample */
    EXPECT_FLOAT_EQ(0.0f, out_buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(0.0f, out_buffer.channel(1)[0]);
    EXPECT_FLOAT_EQ(1.0f, out_buffer.channel(0)[1]);
    EXPECT_FLOAT_EQ(1.0f, out_buffer.channel(1)[1]);
}

TEST_F(TestVst3xWrapper, TestBypassProcessing)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1.0f);
    // The adelay example supports soft bypass.
    EXPECT_TRUE(_module_under_test->_bypass_parameter.supported);
    EXPECT_EQ(BYPASS_PARAM_ID, _module_under_test->_bypass_parameter.id);

    // Set bypass and manually feed the generated RtEvent back to the
    // wrapper processor as event dispatcher is not running
    _module_under_test->set_bypassed(true);
    auto bypass_event = _host_control._dummy_dispatcher.retrieve_event();
    EXPECT_TRUE(bypass_event.get());
    _module_under_test->process_event(bypass_event->to_rt_event(0));
    _module_under_test->process_audio(in_buffer, out_buffer);

    // Manually call the event callback to send the update back to the
    // controller, as eventloop is not running
    _module_under_test->parameter_update_callback(_module_under_test.get(), 0);
    EXPECT_TRUE(_module_under_test->bypassed());

    // Don't test actual bypass processing because the ADelay example
    // doesn't implement that...
}

TEST_F(TestVst3xWrapper, TestEventForwarding)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    Steinberg::Vst::Event note_on_event;
    note_on_event.type = Steinberg::Vst::Event::EventTypes::kNoteOnEvent;
    note_on_event.sampleOffset = 5;
    note_on_event.noteOn.velocity = 1.0f;
    note_on_event.noteOn.channel = 1;
    note_on_event.noteOn.pitch = 46;

    Steinberg::Vst::Event note_off_event;
    note_off_event.type = Steinberg::Vst::Event::EventTypes::kNoteOffEvent;
    note_off_event.sampleOffset = 6;
    note_off_event.noteOff.velocity = 1.0f;
    note_off_event.noteOff.channel = 2;
    note_off_event.noteOff.pitch = 48;

    _module_under_test->_process_data.outputEvents->addEvent(note_on_event);
    _module_under_test->_process_data.outputEvents->addEvent(note_off_event);
    _module_under_test->_forward_events(_module_under_test->_process_data);

    ASSERT_FALSE(_event_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_event_queue.pop(event));
    ASSERT_EQ(RtEventType::NOTE_ON, event.type());
    ASSERT_EQ(5, event.sample_offset());
    ASSERT_EQ(46, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_TRUE(_event_queue.pop(event));
    ASSERT_EQ(RtEventType::NOTE_OFF, event.type());
    ASSERT_EQ(6, event.sample_offset());
    ASSERT_EQ(48, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_FALSE(_event_queue.pop(event));
}

TEST_F(TestVst3xWrapper, TestConfigurationChange)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    _module_under_test->configure(44100.0f);
    ASSERT_FLOAT_EQ(44100, _module_under_test->_sample_rate);
}

TEST_F(TestVst3xWrapper, TestTimeInfo)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    _host_control._transport.set_playing_mode(PlayingMode::PLAYING, false);
    _host_control._transport.set_tempo(120, false);
    _host_control._transport.set_time_signature({3, 4}, false);
    _host_control._transport.set_time(Time(0), 0);
    _host_control._transport.set_time(std::chrono::seconds(2), static_cast<int64_t>(TEST_SAMPLE_RATE) * 2);

    _module_under_test->_fill_processing_context();
    auto context = _module_under_test->_process_data.processContext;
    /* For these numbers to match exactly, we need to choose a time interval which
     * is an integer multiple of AUDIO_CHUNK_SIZE, hence 2 seconds at 48000, which
     * is good up to AUDIO_CHUNK_SIZE = 256 */
    EXPECT_FLOAT_EQ(TEST_SAMPLE_RATE, context->sampleRate);
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE) * 2, context->projectTimeSamples);
    EXPECT_EQ(2'000'000'000, context->systemTime);
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE) * 2, context->continousTimeSamples);
    EXPECT_FLOAT_EQ(4.0f, context->projectTimeMusic);
    EXPECT_FLOAT_EQ(3.0f, context->barPositionMusic);
    EXPECT_FLOAT_EQ(120.0f, context->tempo);
    EXPECT_EQ(3, context->timeSigNumerator);
    EXPECT_EQ(4, context->timeSigDenominator);
}

TEST_F(TestVst3xWrapper, TestParameterHandling)
{
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    auto [status, value] = _module_under_test->parameter_value(DELAY_PARAM_ID);
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(1.0f, value);

    auto event = RtEvent::make_parameter_change_event(_module_under_test->id(), 0, DELAY_PARAM_ID, 0.5f);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);
    // Manually call the event callback to send the update back to the controller, as eventloop is not running
    _module_under_test->parameter_update_callback(_module_under_test.get(), 0);

    std::tie(status, value) = _module_under_test->parameter_value(DELAY_PARAM_ID);
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_FLOAT_EQ(0.5f, value);

    std::string string_repr;
    std::tie(status, string_repr) = _module_under_test->parameter_value_formatted(DELAY_PARAM_ID);
    EXPECT_EQ(ProcessorReturnCode::OK, status);
    EXPECT_EQ("0.5000", string_repr);
}

TEST_F(TestVst3xWrapper, TestGateOutput)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    auto status = _module_under_test->connect_gate_from_processor(2, 0, 46);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    Steinberg::Vst::Event note_on_event;
    note_on_event.type = Steinberg::Vst::Event::EventTypes::kNoteOnEvent;
    note_on_event.sampleOffset = 5;
    note_on_event.noteOn.velocity = 1.0f;
    note_on_event.noteOn.channel = 0;
    note_on_event.noteOn.pitch = 46;

    _module_under_test->_process_data.outputEvents->addEvent(note_on_event);
    _module_under_test->_forward_events(_module_under_test->_process_data);

    ASSERT_FALSE(_event_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_event_queue.pop(event));
    ASSERT_EQ(RtEventType::GATE_EVENT, event.type());
    ASSERT_EQ(0, event.sample_offset());
    ASSERT_EQ(2, event.gate_event()->gate_no());
    ASSERT_TRUE(event.gate_event()->value());

    ASSERT_TRUE(_event_queue.empty());
}

TEST_F(TestVst3xWrapper, TestCVOutput)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    auto status = _module_under_test->connect_cv_from_parameter(DELAY_PARAM_ID, 1);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    int index_unused;
    auto param_queue = _module_under_test->_process_data.outputParameterChanges->addParameterData(DELAY_PARAM_ID, index_unused);
    ASSERT_TRUE(param_queue);
    param_queue->addPoint(5, 0.75, index_unused);

    _module_under_test->_forward_params(_module_under_test->_process_data);

    ASSERT_FALSE(_event_queue.empty());
    RtEvent event;
    ASSERT_TRUE(_event_queue.pop(event));
    ASSERT_EQ(RtEventType::CV_EVENT, event.type());
    ASSERT_EQ(0, event.sample_offset());
    ASSERT_EQ(1, event.cv_event()->cv_id());
    ASSERT_FLOAT_EQ(0.75f, event.cv_event()->value());

    ASSERT_TRUE(_event_queue.empty());
}

TEST_F(TestVst3xWrapper, TestStateHandling)
{
    ChunkSampleBuffer buffer(2);
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    auto desc = _module_under_test->parameter_from_name("Delay");
    ASSERT_TRUE(desc);

    ProcessorState state;
    state.set_bypass(true);
    state.set_program(2);
    state.add_parameter_change(desc->id(), 0.88);

    auto status = _module_under_test->set_state(&state, false);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Check that new values are set and update notification is queued
    EXPECT_FLOAT_EQ(0.88f, _module_under_test->parameter_value(desc->id()).second);
    EXPECT_TRUE(_module_under_test->bypassed());
    auto event = _host_control._dummy_dispatcher.retrieve_event();
    ASSERT_TRUE(event->is_engine_notification());

    // Test setting state with realtime running
    state.set_bypass(false);
    state.set_program(1);
    state.add_parameter_change(desc->id(), 0.44);

    status = _module_under_test->set_state(&state, true);
    ASSERT_EQ(ProcessorReturnCode::OK, status);
    event = _host_control._dummy_dispatcher.retrieve_event();
    _module_under_test->process_event(event->to_rt_event(0));
    _module_under_test->process_audio(buffer, buffer);

    // Check that new values are set
    EXPECT_FLOAT_EQ(0.44f, _module_under_test->parameter_value(desc->id()).second);
    EXPECT_FALSE(_module_under_test->bypassed());

    // Retrive the delete event and execute it to delete the RtState object
    ASSERT_FALSE(_event_queue.empty());
    RtEvent rt_event;
    _event_queue.pop(rt_event);
    auto delete_event = Event::from_rt_event(rt_event, IMMEDIATE_PROCESS);
    ASSERT_TRUE(delete_event);
    static_cast<AsynchronousDeleteEvent*>(delete_event)->execute();
    delete delete_event;
}

TEST_F(TestVst3xWrapper, TestBinaryStateSaving)
{
    ChunkSampleBuffer buffer(2);
    SetUp(PLUGIN_FILE, PLUGIN_NAME);

    auto desc = _module_under_test->parameter_from_name("Delay");
    ASSERT_TRUE(desc);
    float prev_value = _module_under_test->parameter_value(desc->id()).second;

    ProcessorState state = _module_under_test->save_state();
    ASSERT_TRUE(state.has_binary_data());

    // Set a parameter value, the re-apply the state
    auto event = RtEvent::make_parameter_change_event(_module_under_test->id(), 0, desc->id(), 0.5f);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(buffer, buffer);
    _module_under_test->parameter_update_callback(_module_under_test.get(), 0);

    EXPECT_NE(prev_value, _module_under_test->parameter_value(desc->id()).second);

    auto status = _module_under_test->set_state(&state, false);
    ASSERT_EQ(ProcessorReturnCode::OK, status);

    // Check the value has reverted to the previous value
    EXPECT_FLOAT_EQ(prev_value, _module_under_test->parameter_value(desc->id()).second);
}

class TestVst3xUtils : public ::testing::Test
{
protected:
    TestVst3xUtils() {}
};

TEST_F(TestVst3xUtils, TestNoteOnConversion)
{
    auto event = RtEvent::make_note_on_event(ObjectId(0), 12, 1, 45, 0.5f);
    auto vst_event = convert_note_on_event(event.keyboard_event());
    EXPECT_EQ(0, vst_event.busIndex);
    EXPECT_EQ(12, vst_event.sampleOffset);
    EXPECT_EQ(0, vst_event.ppqPosition);
    EXPECT_EQ(0, vst_event.flags);
    EXPECT_EQ(Steinberg::Vst::Event::kNoteOnEvent, vst_event.type);
    EXPECT_EQ(1, vst_event.noteOn.channel);
    EXPECT_EQ(45, vst_event.noteOn.pitch);
    EXPECT_FLOAT_EQ(0.0f, vst_event.noteOn.tuning);
    EXPECT_FLOAT_EQ(0.5f, vst_event.noteOn.velocity);
    EXPECT_EQ(0, vst_event.noteOn.length);
    EXPECT_EQ(-1, vst_event.noteOn.noteId);
}

TEST_F(TestVst3xUtils, TestNoteOffConversion)
{
    auto event = RtEvent::make_note_off_event(ObjectId(0), 12, 1, 45, 0.5f);
    auto vst_event = convert_note_off_event(event.keyboard_event());
    EXPECT_EQ(0, vst_event.busIndex);
    EXPECT_EQ(12, vst_event.sampleOffset);
    EXPECT_EQ(0, vst_event.ppqPosition);
    EXPECT_EQ(0, vst_event.flags);
    EXPECT_EQ(Steinberg::Vst::Event::kNoteOffEvent, vst_event.type);
    EXPECT_EQ(1, vst_event.noteOff.channel);
    EXPECT_EQ(45, vst_event.noteOff.pitch);
    EXPECT_FLOAT_EQ(0.0f, vst_event.noteOff.tuning);
    EXPECT_FLOAT_EQ(0.5f, vst_event.noteOff.velocity);
    EXPECT_EQ(-1, vst_event.noteOff.noteId);
}

TEST_F(TestVst3xUtils, TestAftertouchConversion)
{
    auto event = RtEvent::make_note_aftertouch_event(ObjectId(0), 12, 1, 45, 0.5f);
    auto vst_event = convert_aftertouch_event(event.keyboard_event());
    EXPECT_EQ(0, vst_event.busIndex);
    EXPECT_EQ(12, vst_event.sampleOffset);
    EXPECT_EQ(0, vst_event.ppqPosition);
    EXPECT_EQ(0, vst_event.flags);
    EXPECT_EQ(Steinberg::Vst::Event::kPolyPressureEvent, vst_event.type);
    EXPECT_EQ(1, vst_event.polyPressure.channel);
    EXPECT_EQ(45, vst_event.polyPressure.pitch);
    EXPECT_FLOAT_EQ(0.5f, vst_event.polyPressure.pressure);
    EXPECT_EQ(-1, vst_event.polyPressure.noteId);
}