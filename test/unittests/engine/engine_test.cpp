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
    float in_buffer[AUDIO_CHUNK_SIZE];
    float out_buffer[AUDIO_CHUNK_SIZE];
    std::fill(in_buffer, in_buffer + AUDIO_CHUNK_SIZE, 1);

    SushiBuffer buffer;
    buffer.left_in = in_buffer;
    buffer.left_out = out_buffer;
    buffer.right_in = in_buffer;
    buffer.right_out = out_buffer;
    _module_under_test->process_chunk(&buffer);

    for (auto& i : out_buffer)
    {
        ASSERT_FLOAT_EQ(1, i);
    }
}
