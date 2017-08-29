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

/* No real technical limit, just something arbitrarily high enough */
constexpr int PLUGIN_CHAIN_MAX_CHANNELS = 8;

class PluginChain : public Processor, public EventPipe
{
public:
    PluginChain() : _bfr_1{PLUGIN_CHAIN_MAX_CHANNELS},
                    _bfr_2{PLUGIN_CHAIN_MAX_CHANNELS}
    {
        _max_input_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _max_output_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _current_input_channels = PLUGIN_CHAIN_MAX_CHANNELS;
        _current_output_channels = PLUGIN_CHAIN_MAX_CHANNELS;
    }

    PluginChain(int channels) : _bfr_1{channels},
                                _bfr_2{channels}
    {
        _max_input_channels = channels;
        _max_output_channels = channels;
        _current_input_channels = channels;
        _current_output_channels = channels;
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

    void process_event(Event event) override;

    void process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out);

    void set_bypassed(bool bypassed) override;

    /* Inherited from EventPipe */
    void send_event(Event event) override;

    bool set_input_channels(int channels) override
    {
        if (Processor::set_input_channels(channels))
        {
            update_channel_config();
            return true;
        }
        return false;
    }

    bool set_output_channels(int channels) override
    {
        if (Processor::set_output_channels(channels))
        {
            update_channel_config();
            return true;
        }
        return false;
    }


private:
    /**
     * @brief Loops through the chain of plugins and negotiatiates channel configuration.
     */
    void update_channel_config();

    eastl::vector<Processor*> _chain;
    ChunkSampleBuffer _bfr_1;
    ChunkSampleBuffer _bfr_2;

    // TODO - Maybe use a simpler queue here eventually
    EventFifo _event_buffer;
};





} // namespace engine
} // namespace sushi



#endif //SUSHI_PLUGIN_CHAIN_H
