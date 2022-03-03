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

#include "logging.h"

#include "send_return_factory.h"
#include "send_plugin.h"
#include "return_plugin.h"

namespace sushi {

SendReturnFactory::SendReturnFactory() = default;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("send_ret_factory");

send_plugin::SendPlugin* SendReturnFactory::get_send()
{
    return nullptr;
}

return_plugin::ReturnPlugin* SendReturnFactory::lookup_return_plugin(const std::string& name)
{
    return_plugin::ReturnPlugin* instance = nullptr;
    std::scoped_lock<std::mutex> lock(_return_inst_lock);
    for (const auto i : _return_instances)
    {
        if (i->name() == name)
        {
            instance = i;
            break;
        }
    }
    SUSHI_LOG_INFO("Looked up return plugin {}, {}", name, instance? "found" : "not found");
    return instance;
}

void SendReturnFactory::on_return_destruction(return_plugin::ReturnPlugin* instance)
{
    std::scoped_lock<std::mutex> lock(_return_inst_lock);
    _return_instances.erase(std::remove(_return_instances.begin(), _return_instances.end(), instance), _return_instances.end());
}

std::pair<ProcessorReturnCode, std::shared_ptr<Processor>>
SendReturnFactory::new_instance(const PluginInfo& plugin_info, HostControl& host_control, float sample_rate)
{
    std::shared_ptr<Processor> processor;

    if (plugin_info.uid == send_plugin::SendPlugin::static_uid())
    {
        processor = std::make_shared<send_plugin::SendPlugin>(host_control, this);
    }
    else if (plugin_info.uid == return_plugin::ReturnPlugin::static_uid())
    {
        auto instance = std::make_shared<return_plugin::ReturnPlugin>(host_control, this);
        std::scoped_lock<std::mutex> lock(_return_inst_lock);
        _return_instances.push_back(instance.get());
        processor = instance;
    }

    if (processor)
    {
        auto processor_status = processor->init(sample_rate);
        return {processor_status, processor};
    }
    return {ProcessorReturnCode::ERROR, processor};
}
}// namespace sushi