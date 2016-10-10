#include <algorithm>

#include "gtest/gtest.h"
#include "engine/engine.cpp"

#define private public

constexpr unsigned int SAMPLE_RATE = 44000;
using namespace sushi_engine;

/*
* Enginge tests
*/
class TestEngine : public ::testing::Test
{
protected:
    TestEngine()
    {
    }

    void SetUp()
    {
        _module_under_test = new SushiEngine(SAMPLE_RATE);
    }

    void TearDown()
    {
        delete _module_under_test;
    }
    SushiEngine* _module_under_test;
};

TEST_F(TestEngine, TestInstantiation)
{
    ASSERT_TRUE(_module_under_test);
}

/*
 * Test that 1:s in gives 1:s out
 */
/*TEST_F(TestEngine, TestProcess)
{
    SampleChunkBuffer in_buffer;
    SampleChunkBuffer out_buffer;
    /*float* left = in_buffer.channel(LEFT);
    float* right = in_buffer.channel(RIGHT);
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        left[n] = 1.0f;
        right[n] = 1.0f;
    }*/
   /* _module_under_test->process_chunk(&in_buffer, &out_buffer);
    float* left = out_buffer.channel(LEFT);
    float* right = out_buffer.channel(RIGHT);
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        ASSERT_FLOAT_EQ(1.0f, left[n]);
        ASSERT_FLOAT_EQ(1.0f, right[n]);
    }
}*/
