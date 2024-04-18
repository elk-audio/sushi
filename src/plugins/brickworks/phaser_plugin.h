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
 * @brief Phaser from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef PHASER_PLUGIN_H
#define PHASER_PLUGIN_H

#include <bw_phaser.h>

#include "library/internal_plugin.h"

ELK_PUSH_WARNING
ELK_DISABLE_DOMINANCE_INHERITANCE

namespace sushi::internal::phaser_plugin {

class PhaserPlugin : public InternalPlugin, public UidHelper<PhaserPlugin>
{
public:
    explicit PhaserPlugin(HostControl hostControl);

    ~PhaserPlugin() override = default;

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
    FloatParameterValue* _center;
    FloatParameterValue* _amount;

    bw_phaser_coeffs _phaser_coeffs;
    std::array<bw_phaser_state, MAX_TRACK_CHANNELS> _phaser_states;
};

} // namespace sushi::internal::phaser_plugin

ELK_POP_WARNING

#endif // PHASER_PLUGIN_H
