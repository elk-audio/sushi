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

    void process_chunk(SushiBuffer* buffer) override
    {
        for (unsigned n=0; n<AUDIO_CHUNK_SIZE; n++)
        {
            buffer->left_out[n] = buffer->left_in[n];
            buffer->right_out[n] = buffer->right_in[n];
        }
    }
};

#endif //SUSHI_ENGINE_MOCKUP_H
