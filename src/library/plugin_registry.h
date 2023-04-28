/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Container and Facade for plugin factories.
 *        Currently, one plugin factory is instantiated and stored, per distinct PluginInfo.
 *        This means e.g. if you instantiate an LV2 metronome, and then an LV2 delay, there are 2 factories stored.
 */

#ifndef SUSHI_PLUGIN_REGISTRY_H
#define SUSHI_PLUGIN_REGISTRY_H

#include <unordered_map>

#include "library/processor.h"
#include "library/base_processor_factory.h"
#include "engine/base_engine.h"

namespace sushi {

struct Hash
{
    size_t operator() (const PluginType &type) const
    {
        return std::hash<int>{}(static_cast<int>(type));
    }
};

class PluginRegistry
{
public:
    std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> new_instance(const PluginInfo& plugin_info,
                                                                            HostControl& host_control,
                                                                            float sample_rate);

private:
    std::unordered_map<PluginType, std::unique_ptr<BaseProcessorFactory>, Hash> _factories;
};

} // end namespace sushi

#endif //SUSHI_PLUGIN_REGISTRY_H
