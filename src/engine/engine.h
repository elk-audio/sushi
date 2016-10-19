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

namespace sushi {
namespace engine {

enum channel_id
{
    LEFT = 0,
    RIGHT,
    MAX_CHANNELS,
};

class BaseEngine
{
public:
    BaseEngine(int sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~BaseEngine()
    {}

    int sample_rate()
    {
        return _sample_rate;
    }

    int n_channels()
    {
        return MAX_CHANNELS;
    }

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;

protected:
    int _sample_rate;
};


class AudioEngine : public BaseEngine
{
public:
    AudioEngine(int sample_rate);

    ~AudioEngine();

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

protected:
    std::vector<std::vector<std::unique_ptr<StompBox>>> _audio_graph{MAX_CHANNELS};
};

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
