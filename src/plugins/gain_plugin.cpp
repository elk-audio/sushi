#include <cassert>

#include "gain_plugin.h"

namespace sushi {
namespace gain_plugin {

GainPlugin::GainPlugin()
{}

GainPlugin::~GainPlugin()
{}

StompBoxStatus GainPlugin::init(const StompBoxConfig &configuration)
{
    _configuration = configuration;
    // For now, leave it at linear gain, later we should use log gain.
    _gain_parameter = configuration.controller->register_float_parameter("Gain", "gain", 0, 24, 0);

    return StompBoxStatus::OK;
}

void GainPlugin::set_parameter(int parameter_id, float value)
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

void GainPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, assume equal number of channels in/out */
    assert(in_buffer->channel_count() == out_buffer->channel_count());

    /* With SampleBuffer operations */
    float gain = _gain_parameter->value();
    out_buffer->clear();
    out_buffer->add_with_gain(*in_buffer, gain);
}


}// namespace gain_plugin
}// namespace sushi