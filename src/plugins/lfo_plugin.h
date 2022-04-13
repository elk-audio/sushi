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

#ifndef LFO_PLUGIN_H
#define LFO_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi {
namespace lfo_plugin {

class LfoPlugin : public InternalPlugin, public UidHelper<LfoPlugin>
{
public:
    explicit LfoPlugin(HostControl host_control);

    ~LfoPlugin() override;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    float _phase{0};
    float _buffers_per_second{0};
    FloatParameterValue* _freq_parameter;
    FloatParameterValue* _out_parameter;
};

}// namespace lfo_plugin
}// namespace sushi
#endif // LFO_PLUGIN_H