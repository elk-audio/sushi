/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Factory for LV2 processors.
 */

#include "lv2_processor_factory.h"
#include "logging.h"

#ifdef SUSHI_BUILD_WITH_LV2
#include "lv2_wrapper.h"
#endif

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

#ifdef SUSHI_BUILD_WITH_LV2

Lv2ProcessorFactory::~Lv2ProcessorFactory() = default;

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> Lv2ProcessorFactory::new_instance(const PluginInfo& plugin_info,
                                                                                             HostControl& host_control,
                                                                                             float sample_rate)
{
    auto world = _world.lock();
    if (!world)
    {
        world = std::make_shared<LilvWorldWrapper>();
        world->create_world();
        if (world->world() == nullptr)
        {
            SUSHI_LOG_ERROR("Failed to initialize Lilv World");
            return {ProcessorReturnCode::SHARED_LIBRARY_OPENING_ERROR, nullptr};
        }
        _world = world;
    }
    auto processor = std::make_shared<lv2::LV2_Wrapper>(host_control, plugin_info.path, world);
    auto processor_status = processor->init(sample_rate);
    return {processor_status, processor};
}

#else //SUSHI_BUILD_WITH_LV2

Lv2ProcessorFactory::~Lv2ProcessorFactory() = default;

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> Lv2ProcessorFactory::new_instance([[maybe_unused]] const PluginInfo& plugin_info,
                                                                                             [[maybe_unused]] HostControl& host_control,
                                                                                             [[maybe_unused]] float sample_rate)

{
    SUSHI_LOG_ERROR("Sushi was not built with support for LV2 plugins");
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, nullptr};
}

#endif //SUSHI_BUILD_WITH_LV2

} // end namespace lv2
} // end namespace sushi
