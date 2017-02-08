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
#include "library/processor.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

class PluginChain : public Processor
{
public:
    PluginChain() = default;
    ~PluginChain() = default;
    MIND_DECLARE_NON_COPYABLE(PluginChain)
    /**
     * @brief Adds a plugin to the end of the chain.
     * @param The plugin to add.
     */
    void add(Processor* processor)
    {
        _chain.push_back(processor);
    }
    /**
     * @brief handles events sent to this processor only and not sub-processors
     */
    void process_event(BaseEvent* /*event*/) override {}

    /**
     * @brief Process the entire chain and store the result in out.
     * @param in input buffer.
     * @param out output buffer.
     */
    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
    {
        _bfr_1 = in;
        for (auto &plugin : _chain)
        {
            plugin->process_audio(_bfr_1, _bfr_2);
            std::swap(_bfr_1, _bfr_2);
        }
        out = _bfr_1;
    }

private:
    eastl::vector<Processor*> _chain;
    SampleBuffer<AUDIO_CHUNK_SIZE> _bfr_1{1};
    SampleBuffer<AUDIO_CHUNK_SIZE> _bfr_2{1};
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
