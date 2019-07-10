#include <cassert>

#include "lfo_plugin.h"

namespace sushi {
namespace lfo_plugin {

static const std::string DEFAULT_NAME = "sushi.testing.lfo";
static const std::string DEFAULT_LABEL = "Lfo";

LfoPlugin::LfoPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = 8;
    _max_output_channels = 8;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _freq_parameter = register_float_parameter("freq", "Frequency", 1.0f, 0.001f, 10.0f);
    _out_parameter = register_float_parameter("out", "Lfo Out", 0.5f, 0.0f, 1.0f);

    assert(_freq_parameter && _out_parameter);
}

LfoPlugin::~LfoPlugin() = default;

ProcessorReturnCode LfoPlugin::init(float sample_rate)
{
    _buffers_per_second = sample_rate / AUDIO_CHUNK_SIZE;
    return ProcessorReturnCode::OK;
}

void LfoPlugin::configure(float sample_rate)
{
    _buffers_per_second = sample_rate / AUDIO_CHUNK_SIZE;
}

void LfoPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    _phase += _freq_parameter->value() * M_PI / _buffers_per_second;
    this->set_parameter_and_notify(_out_parameter, (std::sin(_phase) + 1) * 0.5f);
}

}// namespace gain_plugin
}// namespace sushi