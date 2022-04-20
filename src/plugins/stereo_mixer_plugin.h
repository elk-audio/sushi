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

#ifndef SUSHI_STEREO_MIXER_PLUGIN_H
#define SUSHI_STEREO_MIXER_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/value_smoother.h"


namespace sushi {
namespace stereo_mixer_plugin {

class StereoMixerPlugin : public InternalPlugin, public UidHelper<StereoMixerPlugin>
{
public:
    StereoMixerPlugin(HostControl host_control);

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer& in_buffer,ChunkSampleBuffer& out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _ch1_pan;
    FloatParameterValue* _ch1_gain;
    FloatParameterValue* _ch1_invert_phase;
    ValueSmootherFilter<float> _ch1_left_gain_smoother;
    ValueSmootherFilter<float> _ch1_right_gain_smoother;

    FloatParameterValue* _ch2_pan;
    FloatParameterValue* _ch2_gain;
    FloatParameterValue* _ch2_invert_phase;
    ValueSmootherFilter<float> _ch2_left_gain_smoother;
    ValueSmootherFilter<float> _ch2_right_gain_smoother;
};

} // namespace stereo_mixer_plugin
} // namespace sushi


#endif // SUSHI_STEREO_MIXER_PLUGIN_H
