#ifndef SUSHI_ENGINE_MOCKUP_H
#define SUSHI_ENGINE_MOCKUP_H

#include "engine/engine.h"

using namespace sushi;
using namespace sushi::engine;

// Bypass processor
class EngineMockup : public BaseEngine
{
public:
    EngineMockup(unsigned int sample_rate) :
        BaseEngine(sample_rate)
    {}

    ~EngineMockup()
    {}

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE> *in_buffer ,SampleBuffer<AUDIO_CHUNK_SIZE> *out_buffer) override
    {
        *out_buffer = *in_buffer;
    }

    EngineReturnStatus send_rt_event(BaseEvent* /*event*/)
    {
        return EngineReturnStatus::OK;
    }

};

#endif //SUSHI_ENGINE_MOCKUP_H
