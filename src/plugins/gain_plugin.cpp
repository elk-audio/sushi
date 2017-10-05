#include <cassert>

#include "gain_plugin.h"

namespace sushi {
namespace gain_plugin {

GainPlugin::GainPlugin()
{
    _max_input_channels = MAX_CHANNELS;
    _max_output_channels = MAX_CHANNELS;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _gain_parameter = register_float_parameter("gain", "Gain", 0.0f, -120.0f, 120.0f,
                                               new dBToLinPreProcessor(-120.0f, 120.0f));
    assert(_gain_parameter);
}

GainPlugin::~GainPlugin()
{}

bool GainPlugin::set_input_channels(int channels)
{
    if (Processor::set_input_channels(channels))
    {
        _current_output_channels = channels;
        _max_output_channels = channels;
        return true;
    }
    return false;
}

void GainPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    float gain = _gain_parameter->value();
    if (!_bypassed)
    {
        out_buffer.clear();
        out_buffer.add_with_gain(in_buffer, gain);
    } else
    {
        out_buffer = in_buffer;
    }
}


}// namespace gain_plugin
}// namespace sushi