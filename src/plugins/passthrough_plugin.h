/**
 * @brief Example unit gain plugin.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Passes audio unprocessed from input to output
 */

#ifndef PASSTHROUGH_PLUGIN_H
#define PASSTHROUGH_PLUGIN_H

#include "library/plugin_manager.h"

namespace sushi {
namespace passthrough_plugin {

class PassthroughPlugin : public InternalPlugin
{
public:
    PassthroughPlugin();

    ~PassthroughPlugin();

    const std::string unique_id() override
    {
        return std::string("sushi.testing.passthrough");
    }

    // void process_event(BaseEvent* /*event*/) {}

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

};

}// namespace passthrough_plugin
}// namespace sushi
#endif //PASSTHROUGH_PLUGIN_H
