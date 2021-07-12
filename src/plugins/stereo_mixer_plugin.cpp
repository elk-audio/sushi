#include "stereo_mixer_plugin.h"

namespace sushi {
namespace stereo_mixer_plugin {

StereoMixerPlugin::StereoMixerPlugin(HostControl HostControl): InternalPlugin(HostControl)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_name(DEFAULT_LABEL);

    _left_pan = register_float_parameter("left_pan", "Left Pan", "",
                                         -1.0, -1.0, 1.0, nullptr);
    _left_gain = register_float_parameter("left_gain", "Left Gain", "",
                                          1.0f, 0.0f, 1.0f, nullptr);
    _left_invert_phase = register_float_parameter("left_invert_phase", "Left Invert Phase", "",
                                                  0.0f, 0.0f, 1.0f, nullptr);
    _right_pan = register_float_parameter("right_pan", "Right Pan", "",
                                         -1.0, -1.0, 1.0, nullptr);
    _right_gain = register_float_parameter("right_gain", "Right Gain", "",
                                          1.0f, 0.0f, 1.0f, nullptr);
    _right_invert_phase = register_float_parameter("right_invert_phase", "Right Invert Phase", "",
                                                  0.0f, 0.0f, 1.0f, nullptr);
    assert(_left_pan);
    assert(_left_gain);
    assert(_left_invert_phase);
    assert(_right_pan);
    assert(_right_gain);
    assert(_right_invert_phase);
}

void StereoMixerPlugin::process_audio(const ChunkSampleBuffer& input_buffer,
                                     ChunkSampleBuffer& output_buffer)
{
    output_buffer.add(input_buffer);
}

} // namespace stereo_mixer_plugin
} // namespace sushi

