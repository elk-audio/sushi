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
 * @brief Flanger from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef FLANGER_PLUGIN_H
#define FLANGER_PLUGIN_H

#include <vector>

#include <bw_chorus.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace flanger_plugin {

class FlangerPlugin : public InternalPlugin, public UidHelper<FlangerPlugin>
{
public:
    explicit FlangerPlugin(HostControl hostControl);

    ~FlangerPlugin() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager{false, 100};
    float _sample_rate{0};

    FloatParameterValue* _rate;
    FloatParameterValue* _amount;

    bw_chorus_coeffs _chorus_coeffs;
    std::array<bw_chorus_state, MAX_TRACK_CHANNELS> _chorus_states;
    std::array<std::vector<std::byte>, MAX_TRACK_CHANNELS> _delay_mem_areas;
};

}// namespace flanger_plugin
}// namespace sushi
#endif // FLANGER_PLUGIN_H
