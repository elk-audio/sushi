#include "gtest/gtest.h"

#include "test_utils.h"
#include "library/event_fifo.h"
#include "library/vst3x_host_app.cpp"
#include "library/vst3x_utils.cpp"

#define private public
#include "library/vst3x_wrapper.cpp"

using namespace sushi;
using namespace sushi::vst3;

char PLUGIN_FILE[] = "../VST3/adelay.vst3";
char PLUGIN_NAME[] = "ADelay";

char SYNTH_PLUGIN_FILE[] = "../VST3/mda-vst3.vst3";
char SYNTH_PLUGIN_NAME[] = "mda JX10";

constexpr int DELAY_PARAM_ID = 100;

/* Quick test to test plugin loading */
TEST(TestVst3xPluginLoader, test_load_plugin)
{
    char* full_test_plugin_path = realpath(PLUGIN_FILE, NULL);
    PluginLoader module_under_test(full_test_plugin_path, PLUGIN_NAME);
    bool success;
    PluginInstance instance;
    std::tie(success, instance) = module_under_test.load_plugin();
    ASSERT_TRUE(success);
    ASSERT_TRUE(instance.processor());
    ASSERT_TRUE(instance.component());
    ASSERT_TRUE(instance.controller());

    free(full_test_plugin_path);
}

/* Test that nothing breaks if the plugin is not found */
TEST(TestVst3xPluginLoader, test_load_plugin_from_erroneous_filename)
{
    /* Non existing library */
    PluginLoader module_under_test("/usr/lib/lxvst/no_plugin.vst3", PLUGIN_NAME);
    bool success;
    PluginInstance instance;
    std::tie(success, instance) = module_under_test.load_plugin();
    ASSERT_FALSE(success);

    /* Existing library but non-existing plugin */
    char* full_test_plugin_path = realpath(PLUGIN_FILE, NULL);
    module_under_test = PluginLoader(full_test_plugin_path, "NoPluginWithThisName");
    std::tie(success, instance) = module_under_test.load_plugin();
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
        _module_under_test = new Vst3xWrapper(full_plugin_path, plugin_name);
        free(full_plugin_path);

        auto ret = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, ret);
        _module_under_test->set_enabled(true);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    Vst3xWrapper* _module_under_test;
};

TEST_F(TestVst3xWrapper, test_load_and_init_plugin)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ASSERT_TRUE(_module_under_test);
    EXPECT_EQ("ADelay", _module_under_test->name());

    auto parameters = _module_under_test->all_parameters();
    EXPECT_EQ(2u, parameters.size());
    EXPECT_EQ("Bypass", parameters[0]->name());
    EXPECT_EQ("Delay", parameters[1]->name());
}

TEST_F(TestVst3xWrapper, test_processing)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    ChunkSampleBuffer in_buffer(2);
    ChunkSampleBuffer out_buffer(2);
    test_utils::fill_sample_buffer(in_buffer, 1);
    /* Set delay to 0 */
    auto event = Event::make_parameter_change_event(0u, 0, DELAY_PARAM_ID, 0.0f);

    _module_under_test->set_enabled(true);
    _module_under_test->process_event(event);
    _module_under_test->process_audio(in_buffer, out_buffer);

    /* Minimum delay will still be 1 sample */
    EXPECT_FLOAT_EQ(0.0f, out_buffer.channel(0)[0]);
    EXPECT_FLOAT_EQ(0.0f, out_buffer.channel(1)[0]);
    EXPECT_FLOAT_EQ(1.0f, out_buffer.channel(0)[1]);
    EXPECT_FLOAT_EQ(1.0f, out_buffer.channel(1)[1]);
}

TEST_F(TestVst3xWrapper, test_event_forwarding)
{
    SetUp(PLUGIN_FILE, PLUGIN_NAME);
    EventFifo queue;
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
    Event event;
    ASSERT_TRUE(queue.pop(event));
    ASSERT_EQ(EventType::NOTE_ON, event.type());
    ASSERT_EQ(5, event.sample_offset());
    ASSERT_EQ(46, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_TRUE(queue.pop(event));
    ASSERT_EQ(EventType::NOTE_OFF, event.type());
    ASSERT_EQ(6, event.sample_offset());
    ASSERT_EQ(48, event.keyboard_event()->note());
    ASSERT_FLOAT_EQ(1.0f, event.keyboard_event()->velocity());

    ASSERT_FALSE(queue.pop(event));
}
