#include <algorithm>
#include <cassert>

#include "passthrough_plugin.h"

namespace sushi {
namespace passthrough_plugin {

PassthroughPlugin::PassthroughPlugin()
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
}

PassthroughPlugin::~PassthroughPlugin()
{}

void
PassthroughPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    /* Pass keyboard data/midi through */
    while (!_event_queue.empty())
    {
        Event event;
        if (_event_queue.pop(event))
        {
            output_event(event);
        }
    }
}


}// namespace passthrough_plugin
}// namespace sushi