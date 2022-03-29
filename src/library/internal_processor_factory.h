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
 * @brief Factory for Internal processors.
 */

#ifndef SUSHI_INTERNAL_PROCESSOR_FACTORY_H
#define SUSHI_INTERNAL_PROCESSOR_FACTORY_H

#include "library/base_processor_factory.h"

namespace sushi {

class BaseProcessorFactory;

class InternalProcessorFactory : public BaseProcessorFactory
{
public:
    InternalProcessorFactory();

    ~InternalProcessorFactory() override = default;

    std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> new_instance(const sushi::engine::PluginInfo &plugin_info,
                                                                            HostControl& host_control,
                                                                            float sample_rate) override;
private:
    /**
     * @brief Instantiate a plugin instance of a given type
     * @param uid String unique id
     * @param host_control
     * @return Pointer to plugin instance if uid is valid, nullptr otherwise
     */
    std::shared_ptr<Processor> _create_internal_plugin(const std::string& uid, HostControl& host_control);

    std::unique_ptr<BaseProcessorFactory> _send_return_factory;
};

} // end namespace sushi

#endif //SUSHI_INTERNAL_PROCESSOR_FACTORY_H
