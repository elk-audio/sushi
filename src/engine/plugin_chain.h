/**
 * @Brief Wrapper class to hold a single input - single output chain of processing plugins
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_PLUGIN_CHAIN_H
#define SUSHI_PLUGIN_CHAIN_H

#include <memory>
#include <cassert>

#include "library/sample_buffer.h"
#include "library/processor.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

/* for now, chains have at most stereo capability */
constexpr int PLUGIN_CHAIN_MAX_CHANNELS = 2;

class PluginChain : public Processor
{
public:
    PluginChain()
    {
        _max_input_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _max_output_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _current_input_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _current_output_channels = PLUGIN_CHAIN_MAX_CHANNELS;
    }

    ~PluginChain() = default;
    MIND_DECLARE_NON_COPYABLE(PluginChain)

    /**
     * @brief Adds a plugin to the end of the chain.
     * @param The plugin to add.
     */
    void add(Processor* processor);

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
        /* Alias the internal buffers to get the right channel count */
        ChunkSampleBuffer in_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_1, 0, _current_input_channels);
        ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_2, 0, _current_input_channels);
        in_bfr.clear();
        in_bfr.add(in);
        for (auto &plugin : _chain)
        {
            plugin->process_audio(in_bfr, out_bfr);
            std::swap(in_bfr, out_bfr);
        }
        /* Yes, it is in_bfr buffer here. Either it was swapped with out_bfr or the
         * processing chain was empty */
        out.add(in_bfr);
    }

    void set_input_channels(int channels) override
    {
        Processor::set_input_channels(channels);
        update_channel_config();
    }

    void set_output_channels(int channels) override
    {
        Processor::set_output_channels(channels);
        update_channel_config();
    }


private:
    /**
     * @brief Loops through the chain of plugins and negotiatiates channel configuration.
     */
    void update_channel_config();

    eastl::vector<Processor*> _chain;
    ChunkSampleBuffer _bfr_1{PLUGIN_CHAIN_MAX_CHANNELS};
    ChunkSampleBuffer _bfr_2{PLUGIN_CHAIN_MAX_CHANNELS};
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
