/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Stereo mixer
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_STEREO_MIXER_PLUGIN_H
#define SUSHI_STEREO_MIXER_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/value_smoother.h"

ELK_PUSH_WARNING
ELK_DISABLE_DOMINANCE_INHERITANCE

namespace sushi::internal::stereo_mixer_plugin {

class Accessor;

class StereoMixerPlugin : public InternalPlugin, public UidHelper<StereoMixerPlugin>
{
public:
    explicit StereoMixerPlugin(HostControl host_control);

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer& in_buffer,ChunkSampleBuffer& out_buffer) override;

    static std::string_view static_uid();

private:
    friend Accessor;

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

class Accessor
{
public:
    explicit Accessor(StereoMixerPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] ValueSmootherFilter<float>& ch1_left_gain_smoother()
    {
        return _plugin._ch1_left_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch1_right_gain_smoother()
    {
        return _plugin._ch1_right_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch2_left_gain_smoother()
    {
        return _plugin._ch2_left_gain_smoother;
    }

    [[nodiscard]] ValueSmootherFilter<float>& ch2_right_gain_smoother()
    {
        return _plugin._ch2_right_gain_smoother;
    }

    [[nodiscard]] FloatParameterValue* ch1_pan()
    {
        return _plugin._ch1_pan;
    }

    [[nodiscard]] FloatParameterValue* ch1_gain()
    {
        return _plugin._ch1_gain;
    }

    [[nodiscard]] FloatParameterValue* ch1_invert_phase()
    {
        return _plugin._ch1_invert_phase;
    }

    [[nodiscard]] FloatParameterValue* ch2_pan()
    {
        return _plugin._ch2_pan;
    }

    [[nodiscard]] FloatParameterValue* ch2_gain()
    {
        return _plugin._ch2_gain;
    }

    [[nodiscard]] FloatParameterValue* ch2_invert_phase()
    {
        return _plugin._ch2_invert_phase;
    }

private:
    StereoMixerPlugin& _plugin;
};

} // end namespace sushi::internal::stereo_mixer_plugin

ELK_POP_WARNING

#endif // SUSHI_STEREO_MIXER_PLUGIN_H
