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
 * @brief Wah from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef WAH_PLUGIN_H
#define WAH_PLUGIN_H

#include <bw_wah.h>

#include "library/internal_plugin.h"

namespace sushi::internal::wah_plugin {

class WahPlugin : public InternalPlugin, public UidHelper<WahPlugin>
{
public:
    explicit WahPlugin(HostControl hostControl);

    ~WahPlugin() override = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager{false, std::chrono::milliseconds(30)};
    float _sample_rate{0};

    FloatParameterValue* _wah;

    bw_wah_coeffs _wah_coeffs;
    std::array<bw_wah_state, MAX_TRACK_CHANNELS> _wah_states;
};

} // namespace sushi::internal::wah_plugin

#endif // WAH_PLUGIN_H
