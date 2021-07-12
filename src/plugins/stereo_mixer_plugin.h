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

namespace sushi {
namespace stereo_mixer_plugin {

namespace {
    constexpr int MAX_CHANNELS_SUPPORTED = 2;
    constexpr char DEFAULT_NAME[] = "sushi.testing.stereo_mixer";
    constexpr char DEFAULT_LABEL[] = "Stereo Mixer";
}

class StereoMixerPlugin : public InternalPlugin
{
public:
    StereoMixerPlugin(HostControl host_control);

    void process_audio(const ChunkSampleBuffer& in_buffer,ChunkSampleBuffer& out_buffer) override;

private:
    FloatParameterValue* _left_pan;
    FloatParameterValue* _left_gain;
    FloatParameterValue* _left_invert_phase;

    FloatParameterValue* _right_pan;
    FloatParameterValue* _right_gain;
    FloatParameterValue* _right_invert_phase;
};

} // namespace stereo_mixer_plugin
} // namespace sushi


#endif // SUSHI_STEREO_MIXER_PLUGIN_H
