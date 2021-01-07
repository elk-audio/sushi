/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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

#include "vst3x_processor_factory.h"

namespace sushi {
namespace vst3 {

sushi::vst3::Vst3xProcessorFactory::Vst3xProcessorFactory() {}

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
sushi::vst3::Vst3xProcessorFactory::new_instance(const sushi::engine::PluginInfo &plugin_info,
                                                 sushi::HostControl &host_control, float sample_rate)
{
    auto processor = std::make_shared<Vst3xWrapper>(host_control,
                                                    plugin_info.path,
                                                    plugin_info.uid);
    auto processor_status = processor->init(sample_rate);
    return {processor_status, processor};
}

} // end namespace vst3
} // end namespace sushi
