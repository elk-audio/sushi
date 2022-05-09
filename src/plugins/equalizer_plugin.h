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
 * @brief 1 band equaliser plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef EQUALIZER_PLUGIN_H
#define EQUALIZER_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/biquad_filter.h"

namespace sushi {
namespace equalizer_plugin {

constexpr int MAX_CHANNELS_SUPPORTED = 2;

class EqualizerPlugin : public InternalPlugin, public UidHelper<EqualizerPlugin>
{
public:
    EqualizerPlugin(HostControl hostControl);

    ~EqualizerPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    void _reset_filters();

    float _sample_rate;
    std::array<dsp::biquad::BiquadFilter, MAX_CHANNELS_SUPPORTED> _filters;

    FloatParameterValue* _frequency;
    FloatParameterValue* _gain;
    FloatParameterValue* _q;
};

}// namespace equalizer_plugin
}// namespace sushi
#endif // EQUALIZER_PLUGIN_H