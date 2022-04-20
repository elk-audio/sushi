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
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Factory class to create send and return plugins and manage the shared resources
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SEND_RETURN_FACTORY_H
#define SUSHI_SEND_RETURN_FACTORY_H

#include <mutex>

#include "library/base_processor_factory.h"
#include "send_plugin.h"
#include "return_plugin.h"

namespace sushi {

class SendReturnFactory : public BaseProcessorFactory
{
public:
    SendReturnFactory();

    ~SendReturnFactory() = default;

    send_plugin::SendPlugin* get_send();

    return_plugin::ReturnPlugin* lookup_return_plugin(const std::string& name);

    void on_return_destruction(return_plugin::ReturnPlugin* instance);

    // From BaseProcessorFactory
    std::pair<ProcessorReturnCode, std::shared_ptr<Processor>> new_instance(const PluginInfo &plugin_info,
                                                                            HostControl& host_control,
                                                                            float sample_rate) override;

private:
    std::vector<return_plugin::ReturnPlugin*> _return_instances;
    std::mutex _return_inst_lock;
};

}// namespace sushi

#endif //SUSHI_SEND_RETURN_FACTORY_H
