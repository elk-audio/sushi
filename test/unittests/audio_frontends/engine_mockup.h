#ifndef SUSHI_ENGINE_MOCKUP_H
#define SUSHI_ENGINE_MOCKUP_H

#include "engine/engine.h"

using namespace sushi_engine;

// Bypass processor
class EngineMockup : public EngineBase
{
public:
    EngineMockup(unsigned int sample_rate) :
        EngineBase(sample_rate)
    {}

    ~EngineMockup()
    {}

    void process_chunk(SampleChunkBuffer *in_buffer ,SampleChunkBuffer *out_buffer) override
    {
        float* left_out = out_buffer->channel(LEFT);
        float* right_out = out_buffer->channel(RIGHT);
        float* left_in = in_buffer->channel(LEFT);
        float* right_in = in_buffer->channel(RIGHT);
        for (unsigned n=0; n<AUDIO_CHUNK_SIZE; n++)
        {
            left_out[n] = left_in[n];
            right_out[n] = right_in[n];
        }
    }
};

#endif //SUSHI_ENGINE_MOCKUP_H
