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
    out_buffer = in_buffer;
}


}// namespace passthrough_plugin
}// namespace sushi