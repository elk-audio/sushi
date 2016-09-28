#include "gain_plugin.h"

namespace gain_plugin {

GainPlugin::GainPlugin() : _gain(1)
{}

GainPlugin::~GainPlugin()
{}

AudioProcessorStatus GainPlugin::init(const AudioProcessorConfig& configuration)
{
    _configuration = configuration;
    return AudioProcessorStatus::OK;
}

void GainPlugin::set_parameter(unsigned int parameter_id, float value)
{
    switch (parameter_id)
    {
        case gain_parameter_id::GAIN:
        {
            _gain = value;
            break;
        }
    }
}

void GainPlugin::process(const float *in_buffer, float *out_buffer)
{
    for (unsigned int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
    {
        out_buffer[i] = _gain * in_buffer[i];
    }
}


}// namespace gain_plugin