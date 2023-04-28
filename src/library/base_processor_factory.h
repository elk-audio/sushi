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
 * @brief Abstract Factory for processors.
 *        Should be subclassed for each distinct Processor type, e.g. once for VST2, once for VST3, etc.
 */

#ifndef SUSHI_BASE_PROCESSOR_FACTORY_H
#define SUSHI_BASE_PROCESSOR_FACTORY_H

#include "library/processor.h"
#include "engine/base_engine.h"

namespace sushi {

class BaseProcessorFactory
{
public:
    virtual ~BaseProcessorFactory() = default;

    /**
     * @brief Attempts to create a new Processor instance.
     * @param plugin_info
     * @param host_control
     * @param sample_rate
     * @return A pair with ReturnCode, and Processor instance.
     *         If failed, the instance will be nullptr.
     */
    virtual std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> new_instance(const PluginInfo& plugin_info,
                                                                                    HostControl& host_control,
                                                                                    float sample_rate) = 0;
};

} // end namespace sushi

#endif //SUSHI_BASE_PROCESSOR_FACTORY_H
