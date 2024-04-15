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
 * @brief Gain plugin example
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef GAIN_PLUGIN_H
#define GAIN_PLUGIN_H

#include "library/internal_plugin.h"

namespace sushi::internal::gain_plugin {

ELK_PUSH_WARNING
ELK_DISABLE_DOMINANCE_INHERITANCE

class Accessor;

class GainPlugin : public InternalPlugin, public UidHelper<GainPlugin>
{
public:
    explicit GainPlugin(HostControl host_control);

    ~GainPlugin() override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    friend Accessor;

    FloatParameterValue* _gain_parameter;
};

class Accessor
{
public:
    explicit Accessor(GainPlugin& plugin) : _plugin(plugin) {}

    [[nodiscard]] FloatParameterValue* gain_parameter()
    {
        return _plugin._gain_parameter;
    }

private:
    GainPlugin& _plugin;
};

} // end namespace sushi::internal::gain_plugin

ELK_POP_WARNING

#endif // GAIN_PLUGIN_H