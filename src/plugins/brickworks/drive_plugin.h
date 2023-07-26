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
 * @brief Drive from Brickworks library, with internal 2x resampling
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef DRIVE_PLUGIN_H
#define DRIVE_PLUGIN_H

#include <bw_drive.h>
#include <bw_src_int.h>

#include "library/internal_plugin.h"

namespace sushi::internal::drive_plugin {

class DrivePlugin : public InternalPlugin, public UidHelper<DrivePlugin>
{
public:
    explicit DrivePlugin(HostControl hostControl);

    ~DrivePlugin() override = default;

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

    FloatParameterValue* _drive;
    FloatParameterValue* _tone;
    FloatParameterValue* _volume;

    bw_drive_coeffs _drive_coeffs;
    bw_src_int_coeffs _src_up_coeffs;
    bw_src_int_coeffs _src_down_coeffs;
    std::array<bw_drive_state, MAX_TRACK_CHANNELS> _drive_states;
    std::array<bw_src_int_state, MAX_TRACK_CHANNELS> _src_up_states;
    std::array<bw_src_int_state, MAX_TRACK_CHANNELS> _src_down_states;

    ChunkSampleBuffer _tmp_buf{MAX_TRACK_CHANNELS};
};

} // namespace sushi::internal::drive_plugin

#endif // DRIVE_PLUGIN_H
