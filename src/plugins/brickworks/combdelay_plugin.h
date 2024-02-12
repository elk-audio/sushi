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
 * @brief Comb delay from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <vector>

#ifndef COMBDELAY_PLUGIN_H
#define COMBDELAY_PLUGIN_H

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <bw_comb.h>
#pragma clang diagnostic pop

#include "library/internal_plugin.h"

namespace sushi::internal::comb_plugin {

class CombPlugin : public InternalPlugin, public UidHelper<CombPlugin>
{
public:
    explicit CombPlugin(HostControl hostControl);

    ~CombPlugin() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager{false, std::chrono::milliseconds(100)};
    float _sample_rate{0};

    FloatParameterValue* _ff_delay;
    FloatParameterValue* _fb_delay;
    FloatParameterValue* _blend;
    FloatParameterValue* _ff_coeff;
    FloatParameterValue* _fb_coeff;

    bw_comb_coeffs _comb_coeffs;
    std::array<bw_comb_state, MAX_TRACK_CHANNELS> _comb_states;
    std::array<std::vector<std::byte>, MAX_TRACK_CHANNELS> _delay_mem_areas;
};

} // namespace sushi::internal::comb_plugin

#endif // COMBDELAY_PLUGIN_H
