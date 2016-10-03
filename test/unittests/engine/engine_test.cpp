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
    SushiBuffer buffer;
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

TEST(TestSushiBuffer, test_deinterleave_buffer)
{
    float interleaved_buffer[AUDIO_CHUNK_SIZE * 2];
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE*2; n+=2)
    {
        interleaved_buffer[n] = 0.0f;
        interleaved_buffer[n+1] = 1.0f;
    }
    SushiBuffer buffer;
    buffer.input_from_interleaved(&interleaved_buffer[0]);

    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        ASSERT_FLOAT_EQ(0.0f, buffer.left_in[n]);
        ASSERT_FLOAT_EQ(1.0f, buffer.right_in[n]);
    }

}

TEST(TestSushiBuffer, test_interleave_buffer)
{
    SushiBuffer buffer;
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE; n++)
    {
        buffer.left_out[n] = 0.0f;
        buffer.right_out[n] = 1.0f;
    }
    float interleaved_buffer[AUDIO_CHUNK_SIZE * 2];
    buffer.output_to_interleaved(&interleaved_buffer[0]);
    for (unsigned int n=0; n<AUDIO_CHUNK_SIZE*2; n+=2)
    {
        ASSERT_FLOAT_EQ(interleaved_buffer[n], 0.0f);
        ASSERT_FLOAT_EQ(interleaved_buffer[n+1], 1.0f);
    }
    buffer.input_from_interleaved(&interleaved_buffer[0]);
}

