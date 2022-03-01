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
 * @brief Factory for VST2 processors.
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST2X_PROCESSOR_FACTORY_H
#define SUSHI_VST2X_PROCESSOR_FACTORY_H

#include "library/base_processor_factory.h"

namespace sushi {
namespace vst2 {

class Vst2xProcessorFactory : public BaseProcessorFactory
{
public:
    std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> new_instance(const PluginInfo& plugin_info,
                                                                            HostControl& host_control,
                                                                            float sample_rate) override;
};

} // end namespace vst2
} // end namespace sushi

#endif //SUSHI_VST2X_PROCESSOR_FACTORY_H
