#include <algorithm>
#include <cassert>

#include "passthrough_plugin.h"

namespace sushi {
namespace passthrough_plugin {

PassthroughPlugin::PassthroughPlugin()
{}

PassthroughPlugin::~PassthroughPlugin()
{}

void
PassthroughPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* For now, assume equal number of channels in/out */
    assert(in_buffer.channel_count() == out_buffer.channel_count());
    /* Pass audio through */
    out_buffer = in_buffer;

    /* Pass keyboard data/midi through */
    while (!_event_queue.empty())
    {
        output_event(_event_queue.pop());
    }
}


}// namespace passthrough_plugin
}// namespace sushi