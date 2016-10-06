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
TEST_F(TestEngine, TestProcess)
{
    SampleBuffer buffer;
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        buffer.left_in[n] = 1.0f;
        buffer.right_in[n] = 1.0f;
    }
    _module_under_test->process_chunk(&buffer);
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        ASSERT_FLOAT_EQ(1.0f, buffer.left_out[n]);
        ASSERT_FLOAT_EQ(1.0f, buffer.right_out[n]);
    }
}
