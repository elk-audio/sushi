/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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

#include "plugin_registry.h"

#include "internal_processor_factory.h"
#include "vst2x/vst2x_processor_factory.h"
#include "vst3x/vst3x_processor_factory.h"
#include "lv2/lv2_processor_factory.h"

namespace sushi {

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
PluginRegistry::new_instance(const PluginInfo& plugin_info,
                             sushi::HostControl& host_control,
                             float sample_rate)
{
    if (_factories.count(plugin_info.type) == 0)
    {
        switch (plugin_info.type)
        {
            case PluginType::INTERNAL:
            {
                std::unique_ptr<BaseProcessorFactory> new_factory = std::make_unique<InternalProcessorFactory>();
                _factories[plugin_info.type] = std::move(new_factory);
                break;
            }
            case PluginType::VST2X:
            {
                std::unique_ptr<BaseProcessorFactory> new_factory = std::make_unique<vst2::Vst2xProcessorFactory>();
                _factories[plugin_info.type] = std::move(new_factory);
                break;
            }
            case PluginType::VST3X:
            {
                std::unique_ptr<BaseProcessorFactory> new_factory = std::make_unique<vst3::Vst3xProcessorFactory>();
                _factories[plugin_info.type] = std::move(new_factory);
                break;
            }
            case PluginType::LV2:
            {
                std::unique_ptr<BaseProcessorFactory> new_factory = std::make_unique<lv2::Lv2ProcessorFactory>();
                _factories[plugin_info.type] = std::move(new_factory);
                break;
            }
            default:
                return {ProcessorReturnCode::PLUGIN_LOAD_ERROR, nullptr};
        }
    }

    return _factories[plugin_info.type]->new_instance(plugin_info, host_control, sample_rate);
}

} // end namespace sushi