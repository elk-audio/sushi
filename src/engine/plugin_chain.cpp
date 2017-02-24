
#include "plugin_chain.h"
#include <iostream>

namespace sushi {
namespace engine {


void PluginChain::add(Processor* processor)
{
    _chain.push_back(processor);
    update_channel_config();
}


void PluginChain::update_channel_config()
{
    /* No real channel config negotiation here, simply  assume that the channel
     * config for the chain also applies to all processors that are part of it.
     * ie. if the chain is stereo in/stereo out, all processors should also be
     * configured as stereo in/stereo out. */
    if (_chain.empty())
    {
        return;
    }
    for (auto& processor : _chain)
    {
        processor->set_input_channels(_current_input_channels);
        processor->set_output_channels(_current_input_channels);
    }
    return;
}

} // namespace engine
} // namespace sushi
