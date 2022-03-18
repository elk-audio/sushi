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
 * @brief Factory implementation for VST2 processors.
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "vst2x_processor_factory.h"
#include "logging.h"

#ifdef SUSHI_BUILD_WITH_VST2
#include "library/vst2x/vst2x_wrapper.h"
#endif

namespace sushi {
namespace vst2 {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
SUSHI_GET_LOGGER_WITH_MODULE_NAME("Vst2");
#pragma GCC diagnostic pop

#ifdef SUSHI_BUILD_WITH_VST2

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> Vst2xProcessorFactory::new_instance(const PluginInfo& plugin_info,
                                                                                               HostControl& host_control,
                                                                                               float sample_rate)
{
    auto processor = std::make_shared<Vst2xWrapper>(host_control, plugin_info.path);
    auto processor_status = processor->init(sample_rate);
    return {processor_status, processor};
}

#else //SUSHI_BUILD_WITH_VST2

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> Vst2xProcessorFactory::new_instance([[maybe_unused]] const PluginInfo& plugin_info,
                                                                                               [[maybe_unused]] HostControl& host_control,
                                                                                               [[maybe_unused]] float sample_rate)

{
    SUSHI_LOG_ERROR("Sushi was not built with support for VST2 plugins");
    return {ProcessorReturnCode::UNSUPPORTED_OPERATION, nullptr};
}

#endif // SUSHI_BUILD_WITH_VST2

} // end namespace vst2
} // end namespace sushi
