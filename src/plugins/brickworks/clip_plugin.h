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
 * @brief Clip from Brickworks library, with internal 2x resampling
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef CLIP_PLUGIN_H
#define CLIP_PLUGIN_H

#include <bw_clip.h>
#include <bw_src_int.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace clip_plugin {

class ClipPlugin : public InternalPlugin, public UidHelper<ClipPlugin>
{
public:
    explicit ClipPlugin(HostControl hostControl);

    ~ClipPlugin() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager;
    float _sample_rate{0};

    FloatParameterValue* _bias;
    FloatParameterValue* _gain;

    bw_clip_coeffs _clip_coeffs;
    bw_src_int_coeffs _src_up_coeffs;
    bw_src_int_coeffs _src_down_coeffs;
    std::array<bw_clip_state, MAX_TRACK_CHANNELS>   _clip_states;
    std::array<bw_src_int_state, MAX_TRACK_CHANNELS> _src_up_states;
    std::array<bw_src_int_state, MAX_TRACK_CHANNELS> _src_down_states;

    ChunkSampleBuffer _tmp_buf{MAX_TRACK_CHANNELS};
};

}// namespace clip_plugin
}// namespace sushi
#endif // CLIP_PLUGIN_H
