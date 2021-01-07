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

#include "vst2x_processor_factory.h"
#include "library/vst2x/vst2x_wrapper.h"

namespace sushi {

Vst2xProcessorFactory::Vst2xProcessorFactory() {}

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
Vst2xProcessorFactory::new_instance(const sushi::engine::PluginInfo &plugin_info,
                                    sushi::HostControl &host_control,
                                    float sample_rate)
{
    auto processor = std::make_shared<vst2::Vst2xWrapper>(host_control, plugin_info.path);
    auto processor_status = processor->init(sample_rate);
    return {processor_status, processor};
}

}; // end namespace sushi