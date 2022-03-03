/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Stereo mixer
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "stereo_mixer_plugin.h"
#include "library/constants.h"

namespace sushi {
namespace stereo_mixer_plugin {

constexpr char PLUGIN_UID[] = "sushi.testing.stereo_mixer";
constexpr char DEFAULT_LABEL[] = "Stereo Mixer";

constexpr int MAX_CHANNELS_SUPPORTED = 2;

/**
 * @brief Panning calculation using the same law as for the tracks but scaled
 * to keep the gain constant in the default "passthrough" behaviour.
 *
 * @param gain The gain to apply the pan to
 * @param pan The pan amount
 * @return std::pair<float, float> the gain for the <right, left> channel
 */
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

StereoMixerPlugin::StereoMixerPlugin(HostControl HostControl): InternalPlugin(HostControl)
{
    _max_input_channels = MAX_CHANNELS_SUPPORTED;
    _max_output_channels = MAX_CHANNELS_SUPPORTED;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _ch1_pan = register_float_parameter("ch1_pan", "Channel 1 Pan", "",
                                        -1.0, -1.0, 1.0,
                                        Direction::AUTOMATABLE,
                                        nullptr);

    _ch1_gain = register_float_parameter("ch1_gain", "Channel 1 Gain", "",
                                         0.0f, -120.0f, 24.0f,
                                         Direction::AUTOMATABLE,
                                         new dBToLinPreProcessor(-120.0f, 24.0));

    _ch1_invert_phase = register_float_parameter("ch1_invert_phase", "Channel 1 Invert Phase", "",
                                                 0.0f, 0.0f, 1.0f,
                                                 Direction::AUTOMATABLE,
                                                 nullptr);
    _ch2_pan = register_float_parameter("ch2_pan", "Channel 2 Pan", "",
                                        1.0, -1.0, 1.0,
                                        Direction::AUTOMATABLE,
                                        nullptr);

    _ch2_gain = register_float_parameter("ch2_gain", "Channel 2 Gain", "",
                                         0.0f, -120.0f, 24.0f,
                                         Direction::AUTOMATABLE,
                                         new dBToLinPreProcessor(-120.0f, 24.0f));

    _ch2_invert_phase = register_float_parameter("ch2_invert_phase", "Channel 2 Invert Phase", "",
                                                 0.0f, 0.0f, 1.0f,
                                                 Direction::AUTOMATABLE,
                                                 nullptr);

    assert(_ch1_pan);
    assert(_ch1_gain);
    assert(_ch1_invert_phase);
    assert(_ch2_pan);
    assert(_ch2_gain);
    assert(_ch2_invert_phase);

    _ch1_left_gain_smoother.set_direct(1.0f);
    _ch1_right_gain_smoother.set_direct(0.0f);
    _ch2_left_gain_smoother.set_direct(0.0f);
    _ch2_right_gain_smoother.set_direct(1.0f);
}

ProcessorReturnCode StereoMixerPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void StereoMixerPlugin::configure(float sample_rate)
{
    _ch1_left_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    _ch1_right_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    _ch2_left_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    _ch2_right_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
}

void StereoMixerPlugin::process_audio(const ChunkSampleBuffer& input_buffer,
                                     ChunkSampleBuffer& output_buffer)
{
    output_buffer.clear();

    // Calculate parameters
    float invert_ch1 = (_ch1_invert_phase->processed_value() > 0.5f) ? -1.0f : 1.0f;
    auto [ch1_left_gain, ch1_right_gain] = calc_l_r_gain(_ch1_gain->processed_value() * invert_ch1,
                                                         _ch1_pan->processed_value());
    _ch1_left_gain_smoother.set(ch1_left_gain);
    _ch1_right_gain_smoother.set(ch1_right_gain);

    float invert_ch2 = (_ch2_invert_phase->processed_value() > 0.5f) ? -1.0f : 1.0f;
    auto [ch2_left_gain, ch2_right_gain] = calc_l_r_gain(_ch2_gain->processed_value() * invert_ch2,
                                                         _ch2_pan->processed_value());
    _ch2_left_gain_smoother.set(ch2_left_gain);
    _ch2_right_gain_smoother.set(ch2_right_gain);

    if (_bypassed == false)
    {
        // Process gain
        if (input_buffer.channel_count() == 2)
        {
            if (_ch1_left_gain_smoother.stationary() &&
                _ch1_right_gain_smoother.stationary() &&
                _ch2_left_gain_smoother.stationary() &&
                _ch2_right_gain_smoother.stationary())
            {
                output_buffer.add_with_gain(0, 0, input_buffer, ch1_left_gain);
                output_buffer.add_with_gain(1, 0, input_buffer, ch1_right_gain);
                output_buffer.add_with_gain(0, 1, input_buffer, ch2_left_gain);
                output_buffer.add_with_gain(1, 1, input_buffer, ch2_right_gain);
            }
            else // value needs smoothing
            {
                output_buffer.add_with_ramp(0, 0, input_buffer, _ch1_left_gain_smoother.value(), _ch1_left_gain_smoother.next_value());
                output_buffer.add_with_ramp(1, 0, input_buffer, _ch1_right_gain_smoother.value(), _ch1_right_gain_smoother.next_value());
                output_buffer.add_with_ramp(0, 1, input_buffer, _ch2_left_gain_smoother.value(), _ch2_left_gain_smoother.next_value());
                output_buffer.add_with_ramp(1, 1, input_buffer, _ch2_right_gain_smoother.value(), _ch2_right_gain_smoother.next_value());
            }
        }
        else // Input is mono
        {
            output_buffer.add(input_buffer);
        }
    }
    else
    {
        bypass_process(input_buffer, output_buffer);
    }
}

std::string_view StereoMixerPlugin::static_uid()
{
    return PLUGIN_UID;
}

} // namespace stereo_mixer_plugin
} // namespace sushi

