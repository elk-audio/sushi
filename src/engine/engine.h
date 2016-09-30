/**
 * @Brief real time audio processing engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_ENGINE_H
#define SUSHI_ENGINE_H

#include "plugin_interface.h"

#include <vector>
#include <memory>

namespace sushi_engine {

enum channel_id
{
    LEFT = 0,
    RIGHT,
    MAX_CHANNELS,
};

struct SushiBuffer
{
    float *left_in;
    float *right_in;
    float *left_out;
    float *right_out;
};

class EngineBase
{
public:
    EngineBase(unsigned int sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~EngineBase() {}

    virtual void process_chunk(SushiBuffer *buffer) = 0;

protected:
    unsigned int _sample_rate;
};



class SushiEngine : protected EngineBase
{
public:
    SushiEngine(unsigned int sample_rate);

    ~SushiEngine();

    void process_chunk(SushiBuffer *buffer) override;

protected:
    std::vector<std::vector<std::unique_ptr<AudioProcessorBase>>> _audio_graph{MAX_CHANNELS};
};




}; // sushi_engine
#endif //SUSHI_ENGINE_H
