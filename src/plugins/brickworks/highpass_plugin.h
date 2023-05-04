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
 * @brief HighPass from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef HIGHPASS_PLUGIN_H
#define HIGHPASS_PLUGIN_H

#include <bw_hp1.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace highpass_plugin {

class HighPassPlugin : public InternalPlugin, public UidHelper<HighPassPlugin>
{
public:
    HighPassPlugin(HostControl hostControl);

    ~HighPassPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    FloatParameterValue* _frequency;

    bw_hp1_coeffs _hp1_coeffs;
    std::array<bw_hp1_state, MAX_TRACK_CHANNELS> _hp1_states;
};

}// namespace highpass_plugin
}// namespace sushi
#endif // HIGHPASS_PLUGIN_H
