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
    _gain_parameter = configuration.controller->register_float_parameter("Gain", "gain", 0, 24, 0, new FloatdBToLinPreProcessor(24, -24));
    return StompBoxStatus::OK;
}

void GainPlugin::process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, assume equal number of channels in/out */
    assert(in_buffer->channel_count() == out_buffer->channel_count());

    float gain = _gain_parameter->value();
    /* With SampleBuffer operations */
    out_buffer->clear();
    out_buffer->add_with_gain(*in_buffer, gain);
}


}// namespace gain_plugin
}// namespace sushi