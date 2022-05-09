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
 * @brief Simple Cv control plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "lfo_plugin.h"

namespace sushi {
namespace lfo_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.lfo";
constexpr auto DEFAULT_LABEL = "Lfo";

LfoPlugin::LfoPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _freq_parameter = register_float_parameter("freq", "Frequency", "Hz",
                                               1.0f, 0.001f, 10.0f, Direction::AUTOMATABLE);

    _out_parameter = register_float_parameter("out", "Lfo Out", "",
                                              0.5f, 0.0f, 1.0f, Direction::AUTOMATABLE);

    assert(_freq_parameter && _out_parameter);
}

LfoPlugin::~LfoPlugin() = default;

ProcessorReturnCode LfoPlugin::init(float sample_rate)
{
    _buffers_per_second = sample_rate / AUDIO_CHUNK_SIZE;
    return ProcessorReturnCode::OK;
}

void LfoPlugin::configure(float sample_rate)
{
    _buffers_per_second = sample_rate / AUDIO_CHUNK_SIZE;
}

void LfoPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    _phase += _freq_parameter->processed_value() * M_PI / _buffers_per_second;
    this->set_parameter_and_notify(_out_parameter, (std::sin(_phase) + 1) * 0.5f);
}

std::string_view LfoPlugin::static_uid()
{
    return PLUGIN_UID;
}

}// namespace lfo_plugin
}// namespace sushi
