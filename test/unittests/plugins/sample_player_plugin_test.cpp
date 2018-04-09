#include "gtest/gtest.h"

#include "test_utils/test_utils.h"
#include "test_utils/host_control_mockup.h"
#include "library/rt_event_fifo.h"

#define private public

#include "plugins/sample_player_voice.cpp"
#include "plugins/sample_player_plugin.cpp"

using namespace sushi;
using namespace sushi::sample_player_plugin;
using namespace sample_player_voice;

const float SAMPLE_DATA[] = {1.0f, 2.0f, 2.0f, 1.0f, 1.0f};
const int SAMPLE_DATA_LENGTH = sizeof(SAMPLE_DATA) / sizeof(float);

static const std::string SAMPLE_FILE = "Kawai-K11-GrPiano-C4_mono.wav";


/* Test the Voice class */
class TestSamplerVoice : public ::testing::Test
{
protected:
    TestSamplerVoice()
    {
    }
    void SetUp()
    {
        _module_under_test.set_sample(&_sample);
        _module_under_test.set_samplerate(44100);
        _module_under_test.set_envelope(0,0,1,0);
    }

    void TearDown()
    {
    }
    dsp::Sample _sample{SAMPLE_DATA, SAMPLE_DATA_LENGTH};
    Voice _module_under_test;
};

TEST_F(TestSamplerVoice, TestInitialConditions)
{
    EXPECT_FALSE(_module_under_test.active());
    sushi::SampleBuffer<AUDIO_CHUNK_SIZE> buffer(1);
    buffer.clear();
    _module_under_test.render(buffer);
    test_utils::assert_buffer_value(0.0f, buffer);
}

TEST_F(TestSamplerVoice, TestNoteOn)
{
    sushi::SampleBuffer<AUDIO_CHUNK_SIZE> buffer(1);
    buffer.clear();

    _module_under_test.note_on(60, 1.0f, 10);
    _module_under_test.render(buffer);

    float* buf = buffer.channel(0);
    EXPECT_FLOAT_EQ(0.0f, buf[5]);
    EXPECT_FLOAT_EQ(0.0f, buf[9]);
    EXPECT_FLOAT_EQ(1.0f, buf[10]);
    EXPECT_FLOAT_EQ(2.0f, buf[12]);
    EXPECT_FLOAT_EQ(0.0f, buf[15]);
}

/* Test note on and note of during same audio chunk */
TEST_F(TestSamplerVoice, TestNoteOff)
{
    sushi::SampleBuffer<AUDIO_CHUNK_SIZE> buffer(1);
    buffer.clear();

    _module_under_test.note_on(60, 1.0f, 0);
    _module_under_test.note_off(1.0f, 4);
    _module_under_test.render(buffer);

    float* buf = buffer.channel(0);
    EXPECT_FLOAT_EQ(1.0f, buf[0]);
    EXPECT_FLOAT_EQ(2.0f, buf[1]);
    EXPECT_FLOAT_EQ(2.0f, buf[2]);
    EXPECT_FLOAT_EQ(1.0f, buf[3]);
    /* This is where the note should end */
    EXPECT_FLOAT_EQ(0.0f, buf[4]);
}


/* Test the Plugin */
class TestSamplePlayerPlugin : public ::testing::Test
{
protected:
    TestSamplePlayerPlugin()
    {
    }
    void SetUp()
    {
        _module_under_test = new SamplePlayerPlugin(_host_control.make_host_control_mockup());
        ProcessorReturnCode status = _module_under_test->init(48000);
        ASSERT_EQ(ProcessorReturnCode::OK, status);
    }
    void TearDown()
    {
        delete _module_under_test;
    }
    HostControlMockup _host_control;
    SamplePlayerPlugin* _module_under_test;

};

TEST_F(TestSamplePlayerPlugin, TestSampleLoading)
{
    RtEventFifo queue;
    _module_under_test->set_event_output(&queue);
    std::string* path = new std::string(test_utils::get_data_dir_path());
    path->append(SAMPLE_FILE);
    auto sample_ev = RtEvent::make_string_parameter_change_event(0,0,5,path);
    ASSERT_EQ(nullptr, _module_under_test->_sample_buffer);
    _module_under_test->process_event(sample_ev);

    /* Simulate an event dispatcher receieving the event and calling the non-rt callback */
    ASSERT_FALSE(queue.empty());
    RtEvent async_event;
    queue.pop(async_event);
    auto typed_event = async_event.async_work_event();
    int status = typed_event->callback()(typed_event->callback_data(), typed_event->event_id());
    ASSERT_EQ(SampleChangeStatus::SUCCESS, status);
    RtEvent completion_event = RtEvent::make_async_work_completion_event(typed_event->processor_id(),
                                                                         typed_event->event_id(),
                                                                         status);
    _module_under_test->process_event(completion_event);

    /* Sample should now be changed */
    ASSERT_NE(nullptr, _module_under_test->_sample_buffer);

    /* Plugin should have put a delete event on the output queue, just check that it's there */
    ASSERT_FALSE(queue.empty());
}

TEST_F(TestSamplePlayerPlugin, TestProcessing)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    _module_under_test->_sample.set_sample(SAMPLE_DATA, SAMPLE_DATA_LENGTH);
    out_buffer.clear();
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
}

TEST_F(TestSamplePlayerPlugin, TestEventProcessing)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    BlobData data = _module_under_test->load_sample_file(test_utils::get_data_dir_path().append(SAMPLE_FILE));
    ASSERT_NE(0, data.size);
    _module_under_test->_sample.set_sample(reinterpret_cast<float*>(data.data), data.size * sizeof(float));
    out_buffer.clear();
    RtEvent note_on = RtEvent::make_note_on_event(0, 5, 60, 1.0f);
    RtEvent note_on2 = RtEvent::make_note_on_event(0, 50, 65, 1.0f);
    _module_under_test->process_event(note_on);
    _module_under_test->process_event(note_on2);

    _module_under_test->process_audio(in_buffer, out_buffer);
    // assert that something was written to the buffer
    ASSERT_NE(0.0f, out_buffer.channel(0)[10]);
    ASSERT_NE(0.0f, out_buffer.channel(0)[15]);

    // Test that bypass works
    _module_under_test->set_bypassed(true);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);

    // And that we have no hanging notes
    _module_under_test->set_bypassed(false);
    _module_under_test->process_audio(in_buffer, out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);
    delete data.data;
}
