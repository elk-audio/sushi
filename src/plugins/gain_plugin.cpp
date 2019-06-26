#include <cassert>

#include "gain_plugin.h"

namespace sushi {
namespace gain_plugin {

GainPlugin::GainPlugin(HostControl host_control) : InternalPlugin(host_control)
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

void GainPlugin::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    _current_output_channels = channels;
    _max_output_channels = channels;
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
        bypass_process(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
    }
}


}// namespace gain_plugin
}// namespace sushi