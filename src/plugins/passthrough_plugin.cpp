#include <algorithm>

#include "passthrough_plugin.h"

namespace passthrough_plugin {

PassthroughPlugin::PassthroughPlugin()
{}

PassthroughPlugin::~PassthroughPlugin()
{}

AudioProcessorStatus PassthroughPlugin::init(const AudioProcessorConfig& configuration)
{
    return AudioProcessorStatus::OK;
}

void PassthroughPlugin::set_parameter(unsigned int parameter_id, float value)
{}

void PassthroughPlugin::process(const float *in_buffer, float *out_buffer)
{
    std::copy(in_buffer, in_buffer + AUDIO_CHUNK_SIZE, out_buffer);
}


}// namespace passthrough_plugin