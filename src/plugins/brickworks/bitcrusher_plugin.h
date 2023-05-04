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
 * @brief Bitcrusher from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef BITCRUSHER_PLUGIN_H
#define BITCRUSHER_PLUGIN_H

#include <bw_sr_reduce.h>
#include <bw_bd_reduce.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace bitcrusher_plugin {

class BitcrusherPlugin : public InternalPlugin, public UidHelper<BitcrusherPlugin>
{
public:
    BitcrusherPlugin(HostControl hostControl);

    ~BitcrusherPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float /*sample_rate*/) override
    {}

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _samplerate_ratio;
    IntParameterValue* _bit_depth;

    bw_sr_reduce_coeffs _sr_reduce_coeffs;
    bw_bd_reduce_coeffs _bd_reduce_coeffs;
    std::array<bw_sr_reduce_state, MAX_TRACK_CHANNELS> _sr_reduce_states;
};

}// namespace bitcrusher_plugin
}// namespace sushi
#endif // BITCRUSHER_PLUGIN_H
