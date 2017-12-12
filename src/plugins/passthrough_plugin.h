/**
 * @brief Example unit gain plugin.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Passes audio unprocessed from input to output
 */

#ifndef PASSTHROUGH_PLUGIN_H
#define PASSTHROUGH_PLUGIN_H

#include "library/internal_plugin.h"
#include "library/event_fifo.h"


namespace sushi {
namespace passthrough_plugin {

static const std::string DEFAULT_NAME = "sushi.testing.passthrough";
static const std::string DEFAULT_LABEL = "Passthrough";

class PassthroughPlugin : public InternalPlugin
{
public:
    PassthroughPlugin();

    ~PassthroughPlugin();

    void process_event(RtEvent event) override
    {
        _event_queue.push(event);
    };

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

private:
    RtEventFifo _event_queue;
};

}// namespace passthrough_plugin
}// namespace sushi
#endif //PASSTHROUGH_PLUGIN_H
