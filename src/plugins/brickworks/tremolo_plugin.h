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
 * @brief Tremolo from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef TREMOLO_PLUGIN_H
#define TREMOLO_PLUGIN_H

#include <bw_trem.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace tremolo_plugin {

class TremoloPlugin : public InternalPlugin, public UidHelper<TremoloPlugin>
{
public:
    explicit TremoloPlugin(HostControl hostControl);

    ~TremoloPlugin() override = default;

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

    FloatParameterValue* _rate;
    FloatParameterValue* _amount;

    bw_trem_coeffs _trem_coeffs;
    std::array<bw_trem_state, MAX_TRACK_CHANNELS> _trem_states;
};

}// namespace tremolo_plugin
}// namespace sushi
#endif // TREMOLO_PLUGIN_H
