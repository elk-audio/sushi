/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Gain plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "gain_plugin.h"

namespace sushi {
namespace gain_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.gain";
constexpr auto DEFAULT_LABEL = "Gain";

GainPlugin::GainPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _gain_parameter = register_float_parameter("gain", "Gain", "dB",
                                               0.0f, -120.0f, 24.0f,
                                               Direction::AUTOMATABLE,
                                               new dBToLinPreProcessor(-120.0f, 24.0f));
    assert(_gain_parameter);
}

GainPlugin::~GainPlugin() = default;

void GainPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    float gain = _gain_parameter->processed_value();
    if (!_bypassed)
    {
        out_buffer.clear();
        out_buffer.add_with_gain(in_buffer, gain);
    } else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view GainPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace gain_plugin
}// namespace sushi