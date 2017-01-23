#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "plugins/sample_player_voice.cpp"
#include "plugins/sample_player_plugin.cpp"
#include "library/plugin_manager.h"


using namespace sushi;
using namespace sushi::sample_player_plugin;
using namespace sample_player_voice;

const float SAMPLE_DATA[] = {1.0f, 2.0f, 2.0f, 1.0f, 1.0f};
const int SAMPLE_DATA_LENGTH = sizeof(SAMPLE_DATA) / sizeof(float);

/* Test the sample class */
class TestSampleWrapper : public ::testing::Test
{
protected:
    TestSampleWrapper() {}

    Sample _module_under_test{SAMPLE_DATA, SAMPLE_DATA_LENGTH};
};

TEST_F(TestSampleWrapper, TestSampleInterpolation)
{
    // Get exact sample values
    EXPECT_FLOAT_EQ(1.0f, _module_under_test.at(0.0f));
    EXPECT_FLOAT_EQ(2.0f, _module_under_test.at(1.0f));
    // Get interpolated values
    EXPECT_FLOAT_EQ(1.5f, _module_under_test.at(2.5f));
}

/* Test the envelope class */
class TestADSREnvelope : public ::testing::Test
{
protected:
    TestADSREnvelope() {}

    void SetUp()
    {
        _module_under_test.set_samplerate(100);
        _module_under_test.set_parameters(1, 1, 0.5, 1);
    }
    Envelope _module_under_test;
};

TEST_F(TestADSREnvelope, TestNormalOperation)
{
    EXPECT_TRUE(_module_under_test.finished());
    _module_under_test.gate(true);
    EXPECT_FALSE(_module_under_test.finished());

    /* Test Attack phase */
    float level = _module_under_test.tick(50);
    EXPECT_NEAR(0.5f, level, 0.001);
    level = _module_under_test.tick(50);
    /* this should be around the maximum peak */
    EXPECT_NEAR(1.0f, level, 0.001);
    /* Move into the decay phase */
    level = _module_under_test.tick(50);
    EXPECT_NEAR(0.75f, level, 0.001);
    /* Now we should be in the sustain phase */
    level = _module_under_test.tick(200);
    EXPECT_FLOAT_EQ(0.5f, level);

    /* Set gate off and go to decay phase */
    _module_under_test.gate(false);
    level = _module_under_test.tick(50);
    EXPECT_NEAR(0.25f, level, 0.001);
    level = _module_under_test.tick(55);
    EXPECT_FLOAT_EQ(0.0f, level);
    EXPECT_TRUE(_module_under_test.finished());
}

TEST_F(TestADSREnvelope, TestParameterLimits)
{
    _module_under_test.set_parameters(0, 0, 0.5f, 0);
    EXPECT_TRUE(_module_under_test.finished());
    _module_under_test.gate(true);
    EXPECT_FALSE(_module_under_test.finished());
    /* Only 1 state transition per tick, so tick is called twice */
    float level = _module_under_test.tick(2);
    level = _module_under_test.tick(2);
    EXPECT_FLOAT_EQ(0.5f, level);

    /* Reset and test with tick(0) */
    _module_under_test.reset();
    level = _module_under_test.tick(0);
    EXPECT_FLOAT_EQ(0.0f, level);
    EXPECT_FLOAT_EQ(0.0f, _module_under_test.level());
}

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
    Sample _sample{SAMPLE_DATA, SAMPLE_DATA_LENGTH};
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
    EXPECT_FLOAT_EQ(0.0f, buf[20]);
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
        _module_under_test = new SamplePlayerPlugin();
        _manager = new sushi::StompBoxManager(_module_under_test);
        StompBoxConfig c;
        c.sample_rate = 44000;
        c.controller = static_cast<StompBoxController*>(_manager);
        StompBoxStatus status = _module_under_test->init(c);
        ASSERT_EQ(StompBoxStatus::OK, status);
    }

    void TearDown()
    {
        delete _manager;
    }
    StompBox* _module_under_test;
    StompBoxManager* _manager;
};

TEST_F(TestSamplePlayerPlugin, TestProcessing)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    out_buffer.clear();
    _module_under_test->process(&in_buffer, &out_buffer);
    test_utils::assert_buffer_value(0.0f, out_buffer);

}

TEST_F(TestSamplePlayerPlugin, TestEventProcessing)
{
    SampleBuffer<AUDIO_CHUNK_SIZE> in_buffer(1);
    SampleBuffer<AUDIO_CHUNK_SIZE> out_buffer(1);
    out_buffer.clear();
    KeyboardEvent note_on(EventType::NOTE_ON, "", 5, 60, 1.0f);
    KeyboardEvent note_on2(EventType::NOTE_ON, "", 50, 65, 1.0f);
    _module_under_test->process_event(&note_on);
    _module_under_test->process_event(&note_on2);

    _module_under_test->process(&in_buffer, &out_buffer);
    // assert that something was written to the buffer
    ASSERT_NE(0.0f, out_buffer.channel(0)[10]);
    ASSERT_NE(0.0f, out_buffer.channel(0)[15]);

}