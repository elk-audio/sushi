/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief 2nd-order multimode filter from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef MULTIFILTER_PLUGIN_H
#define MULTIFILTER_PLUGIN_H

#include <bw_mm2.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace multi_filter_plugin {

constexpr int MAX_CHANNELS_SUPPORTED = 8;

class MultiFilterPlugin : public InternalPlugin, public UidHelper<MultiFilterPlugin>
{
public:
    MultiFilterPlugin(HostControl hostControl);

    ~MultiFilterPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _frequency;
    FloatParameterValue* _Q;
    FloatParameterValue* _input_coeff;
    FloatParameterValue* _lowpass_coeff;
    FloatParameterValue* _bandpass_coeff;
    FloatParameterValue* _highpass_coeff;

    bw_mm2_coeffs _mm2_coeffs;
    std::array<bw_mm2_state, MAX_CHANNELS_SUPPORTED> _mm2_states;
};

}// namespace multi_filter_plugin
}// namespace sushi
#endif // MULTIFILTER_PLUGIN_H
