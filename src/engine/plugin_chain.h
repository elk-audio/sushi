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
#include "library/event_fifo.h"

#include "EASTL/vector.h"

namespace sushi {
namespace engine {

/* for now, chains have at most stereo capability */
constexpr int PLUGIN_CHAIN_MAX_CHANNELS = 2;

class PluginChain : public Processor, public EventPipe
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
     * @brief Remove a plugin from the chain.
     * @param processor The ObjectId of the processor to remove
     * @return true if the processor was found and succesfully removed, false otherwise
     */
    bool remove(ObjectId processor);

    /**
     * @brief handles events sent to this processor only and not sub-processors
     */
    void process_event(Event event) override;

    /**
     * @brief Process the entire chain and store the result in out.
     * @param in input buffer.
     * @param out output buffer.
     */
    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out);

    /* Inherited from EventPipe */
    void send_event(Event event) override;

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

    // TODO - Maybe use a simpler queue here eventually
    EventFifo _event_buffer;
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
