#include <cassert>

#include "gain_plugin.h"

namespace sushi {
namespace gain_plugin {

GainPlugin::GainPlugin()
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _gain_parameter = register_float_parameter("gain", "Gain", 0.0f, new dBToLinPreProcessor(-120.0f, 120.0f));
    assert(_gain_parameter);
}

GainPlugin::~GainPlugin()
{}

void GainPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    /* For now, assume equal number of channels in/out */
    assert(in_buffer.channel_count() == out_buffer.channel_count());

    float gain = _gain_parameter->value();
    /* With SampleBuffer operations */
    out_buffer.clear();
    out_buffer.add_with_gain(in_buffer, gain);
}


}// namespace gain_plugin
}// namespace sushi