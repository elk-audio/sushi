#include "gtest/gtest.h"

#define private public

#include "test_utils.h"
#include "plugins/sample_player_voice.cpp"
#include "plugins/sample_player_plugin.h"

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