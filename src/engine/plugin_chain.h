/**
 * @Brief Wrapper class to hold a single input - single output chain of processing plugins
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_CHAIN_H
#define SUSHI_PLUGIN_CHAIN_H

#include <memory>

#include "plugin_interface.h"
#include "library/sample_buffer.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

class PluginChain
{
public:
    PluginChain() = default;
    ~PluginChain() = default;
    MIND_DECLARE_NON_COPYABLE(PluginChain)
    /*
     * Add a stompbox instance to the chain. The chain takes ownership of the instance and will
     * destroy it when the chain is destroyed
     */
    void add(StompBox* plugin)
    {
        _chain.push_back(std::unique_ptr<StompBox>(plugin));
    }
    /*
     * Process the entire chain and put the result in out.
     */
    void process(const SampleBuffer<AUDIO_CHUNK_SIZE>& in, SampleBuffer<AUDIO_CHUNK_SIZE>& out)
    {
        _bfr_1 = in;
        for (auto &plugin : _chain)
        {
            plugin->process(&_bfr_1, &_bfr_2);
            std::swap(_bfr_1, _bfr_2);
        }
        out = _bfr_1;
    }

private:
    eastl::vector<std::unique_ptr<StompBox>> _chain;
    SampleBuffer<AUDIO_CHUNK_SIZE> _bfr_1{1};
    SampleBuffer<AUDIO_CHUNK_SIZE> _bfr_2{1};
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
