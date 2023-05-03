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
 * @brief Noise gate from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef NOISE_GATE_PLUGIN_H
#define NOISE_GATE_PLUGIN_H

#include <bw_noise_gate.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace noise_gate_plugin {

constexpr int MAX_CHANNELS_SUPPORTED = 8;

class NoiseGatePlugin : public InternalPlugin, public UidHelper<NoiseGatePlugin>
{
public:
    NoiseGatePlugin(HostControl hostControl);

    ~NoiseGatePlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _threshold;
    FloatParameterValue* _ratio;
    FloatParameterValue* _attack;
    FloatParameterValue* _release;

    bw_noise_gate_coeffs _noise_gate_coeffs;
    bw_noise_gate_state _noise_gate_states[MAX_CHANNELS_SUPPORTED];
};

}// namespace noise_gate_plugin
}// namespace sushi
#endif // NOISE_GATE_PLUGIN_H
