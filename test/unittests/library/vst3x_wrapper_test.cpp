#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "library/rt_event_fifo.h"
#include "library/vst3x_utils.cpp"

#define private public

#include "test_utils/host_control_mockup.h"
#include "library/vst3x_wrapper.cpp"
#include "library/vst3x_host_app.cpp"

using namespace sushi;
using namespace sushi::vst3;

#ifdef NDEBUG
char PLUGIN_FILE[] = "../VST3/Release/adelay.vst3";
#else
char PLUGIN_FILE[] = "../VST3/Debug/adelay.vst3";
#endif
char PLUGIN_NAME[] = "ADelay";

char SYNTH_PLUGIN_FILE[] = "../VST3/mda-vst3.vst3";
char SYNTH_PLUGIN_NAME[] = "mda JX10";

constexpr unsigned int DELAY_PARAM_ID = 100;
constexpr unsigned int BYPASS_PARAM_ID = 101;
constexpr float TEST_SAMPLE_RATE = 48000;


/* Quick test to test plugin loading */
TEST(TestVst3xPluginInstance, TestLoadPlugin)
{
    char* full_test_plugin_path = realpath(PLUGIN_FILE, NULL);
    PluginInstance module_under_test;
    bool success = module_under_test.load_plugin(full_test_plugin_path, PLUGIN_NAME);
    ASSERT_TRUE(success);
    ASSERT_TRUE(module_under_test.processor());
    ASSERT_TRUE(module_under_test.component());
    ASSERT_TRUE(module_under_test.controller());

    free(full_test_plugin_path);
}

/* Test that nothing breaks if the plugin is not found */
TEST(TestVst3xPluginInstance, TestLoadPluginFromErroneousFilename)
{
    /* Non existing library */
    PluginInstance module_under_test;
    bool success = module_under_test.load_plugin("/usr/lib/lxvst/no_plugin.vst3", PLUGIN_NAME);
    ASSERT_FALSE(success);

    /* Existing library but non-existing plugin */
    char* full_test_plugin_path = realpath(PLUGIN_FILE, NULL);
    success = module_under_test.load_plugin(full_test_plugin_path, "NoPluginWithThisName");
    ASSERT_FALSE(success);
    free(full_test_plugin_path);
}

class TestVst3xWrapper : public ::testing::Test
{
protected:
    TestVst3xWrapper()
    {
    }

    void SetUp(char* plugin_file, char* plugin_name)
    {
        char* full_plugin_path = realpath(plugin_file, NULL);
        _module_under_test = new Vst3xWrapper(_host_control.make_host_control_mockup(TEST_SAMPLE_RATE), full_plugin_path, plugin_name);
        free(full_plugin_path);

        auto ret = _module_under_test->init(TEST_SAMPLE_RATE);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
    Vst3xWrapper* _module_under_test;
};

TEST_F(TestVst3xWrapper, TestLoadAndInitPlugin)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ASSERT_TRUE(_module_under_test);
    EXPECT_EQ("ADelay", _module_under_test->name());

    auto parameters = _module_under_test->all_parameters();
    EXPECT_EQ(1u, parameters.size());
    EXPECT_EQ("Delay", parameters[0]->name());
    EXPECT_EQ(DELAY_PARAM_ID, parameters[0]->id());
    EXPECT_TRUE(_module_under_test->_bypass_parameter.supported);
    EXPECT_EQ(BYPASS_PARAM_ID, static_cast<unsigned int>(_module_under_test->_bypass_parameter.id));
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

TEST_F(TestVst3xWrapper, TestEventForwarding)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    RtEventFifo queue;
    _module_under_test->set_event_output(&queue);

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

    ASSERT_FALSE(queue.empty());
    RtEvent event;
    ASSERT_TRUE(queue.pop(event));
    ASSERT_EQ(RtEventType::NOTE_ON, event.type());
    ASSERT_EQ(5, event.sample_offset());
    ASSERT_EQ(46, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_TRUE(queue.pop(event));
    ASSERT_EQ(RtEventType::NOTE_OFF, event.type());
    ASSERT_EQ(6, event.sample_offset());
    ASSERT_EQ(48, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_FALSE(queue.pop(event));
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
    _host_control._transport.set_tempo(120);
    _host_control._transport.set_time_signature({3, 4});
    _host_control._transport.set_time(std::chrono::seconds(1), static_cast<int64_t>(TEST_SAMPLE_RATE));

    _module_under_test->_fill_processing_context();
    auto context = _module_under_test->_process_data.processContext;

    EXPECT_FLOAT_EQ(TEST_SAMPLE_RATE, context->sampleRate);
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE), context->projectTimeSamples);
    EXPECT_EQ(1'000'000'000, context->systemTime);
    EXPECT_EQ(static_cast<int64_t>(TEST_SAMPLE_RATE), context->continousTimeSamples);
    EXPECT_FLOAT_EQ(2.0f, context->projectTimeMusic);
    EXPECT_FLOAT_EQ(0.0f, context->barPositionMusic);
    EXPECT_FLOAT_EQ(120.0f, context->tempo);
    EXPECT_EQ(3, context->timeSigNumerator);
    EXPECT_EQ(4, context->timeSigDenominator);
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