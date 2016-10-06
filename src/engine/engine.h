/**
 * @Brief real time audio processing engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_ENGINE_H
#define SUSHI_ENGINE_H

#include "plugin_interface.h"
#include "library/sample_buffer.h"

#include <vector>
#include <memory>

namespace sushi_engine {

enum channel_id
{
    LEFT = 0,
    RIGHT,
    MAX_CHANNELS,
};

class EngineBase
{
public:
    EngineBase(unsigned int sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~EngineBase() {}

    unsigned int sample_rate()
    {
        return _sample_rate;
    }

    unsigned int n_channels()
    {
        return MAX_CHANNELS;
    }

    virtual void process_chunk(SampleBuffer *buffer) = 0;

protected:
    unsigned int _sample_rate;
};



class SushiEngine : public EngineBase
{
public:
    SushiEngine(unsigned int sample_rate);

    ~SushiEngine();

    void process_chunk(SampleBuffer *buffer) override;

protected:
    std::vector<std::vector<std::unique_ptr<AudioProcessorBase>>> _audio_graph{MAX_CHANNELS};
};

}; // sushi_engine
#endif //SUSHI_ENGINE_H
