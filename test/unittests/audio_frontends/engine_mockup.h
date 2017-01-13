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

    EngineReturnStatus set_stompbox_parameter(const std::string& /*instance_id*/,
                                              const std::string& /*param_id*/,
                                              const float /*value*/) override
    {
        return EngineReturnStatus::OK;
    }

    EngineReturnStatus send_stompbox_event(const std::string& /*instance_id*/,
                                           BaseEvent* /*event*/)
    {
        return EngineReturnStatus::OK;
    }

};

#endif //SUSHI_ENGINE_MOCKUP_H
