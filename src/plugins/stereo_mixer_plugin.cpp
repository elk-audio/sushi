#include <iostream>

#include "stereo_mixer_plugin.h"

namespace sushi {
namespace stereo_mixer_plugin {

namespace {
    constexpr float PAN_GAIN_3_DB = 1.412537f;

    inline std::pair<float, float> calc_l_r_gain(float gain, float pan)
    {
         float left_gain, right_gain;
        if (pan < 0.0f) // Audio panned left
        {
            left_gain = gain * (1.0f + pan - PAN_GAIN_3_DB * pan);
            right_gain = gain * (1.0f + pan);
        }
        else            // Audio panned right
        {
            left_gain = gain * (1.0f - pan);
            right_gain = gain * (1.0f - pan + PAN_GAIN_3_DB * pan);
        }
        return {left_gain / PAN_GAIN_3_DB, right_gain / PAN_GAIN_3_DB};
    }
}

StereoMixerPlugin::StereoMixerPlugin(HostControl HostControl): InternalPlugin(HostControl)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);

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
    output_buffer.clear();

    // Calculate parameters
    float invert_ch1 = (_left_invert_phase->processed_value() > 0.5f) ? -1.0f : 1.0f;
    auto [ch1_left_gain, ch1_right_gain] = calc_l_r_gain(_left_gain->processed_value() * invert_ch1,
                                                         _left_pan->processed_value());
    float invert_ch2 = (_right_invert_phase->processed_value() > 0.5f) ? -1.0f : 1.0f;
    auto [ch2_left_gain, ch2_right_gain] = calc_l_r_gain(_right_gain->processed_value() * invert_ch2,
                                                         _right_pan->processed_value());

    // Process gain
    output_buffer.add_with_gain(0, 0, input_buffer, ch1_left_gain);
    output_buffer.add_with_gain(1, 0, input_buffer, ch1_right_gain);
    output_buffer.add_with_gain(0, 1, input_buffer, ch2_left_gain);
    output_buffer.add_with_gain(1, 1, input_buffer, ch2_right_gain);
}

} // namespace stereo_mixer_plugin
} // namespace sushi

