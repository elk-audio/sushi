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
 * @brief Fuzz from Brickworks library, with internal 2x resampling
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef FUZZ_PLUGIN_H
#define FUZZ_PLUGIN_H

#include <bw_fuzz.h>
#include <bw_src_int.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace fuzz_plugin {

constexpr int MAX_CHANNELS_SUPPORTED = 8;

class FuzzPlugin : public InternalPlugin, public UidHelper<FuzzPlugin>
{
public:
    FuzzPlugin(HostControl hostControl);

    ~FuzzPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _fuzz;
    FloatParameterValue* _volume;

    bw_fuzz_coeffs _fuzz_coeffs;
    bw_src_int_coeffs _src_up_coeffs;
    bw_src_int_coeffs _src_down_coeffs;
    bw_fuzz_state   _fuzz_states[MAX_CHANNELS_SUPPORTED];
    bw_src_int_state _src_up_states[MAX_CHANNELS_SUPPORTED];
    bw_src_int_state _src_down_states[MAX_CHANNELS_SUPPORTED];

    ChunkSampleBuffer _tmp_buf{MAX_CHANNELS_SUPPORTED};
};

}// namespace fuzz_plugin
}// namespace sushi
#endif // FUZZ_PLUGIN_H
