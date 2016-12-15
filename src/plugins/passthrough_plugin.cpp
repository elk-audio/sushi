#include <algorithm>
#include <cassert>

#include "passthrough_plugin.h"

namespace sushi {
namespace passthrough_plugin {

PassthroughPlugin::PassthroughPlugin()
{}

PassthroughPlugin::~PassthroughPlugin()
{}

StompBoxStatus PassthroughPlugin::init(const StompBoxConfig & /* configuration */)
{
    return StompBoxStatus::OK;
}

void
PassthroughPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, assume equal number of channels in/out */
    assert(in_buffer->channel_count() == out_buffer->channel_count());
    for (int i = 0; i < in_buffer->channel_count(); ++i)
    {
        std::copy(in_buffer->channel(i), in_buffer->channel(i) + AUDIO_CHUNK_SIZE, out_buffer->channel(i));
    }
    // or simply:
    // *out_buffer = in_buffer;
}


}// namespace passthrough_plugin
}// namespace sushi