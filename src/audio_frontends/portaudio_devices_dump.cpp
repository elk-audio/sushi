/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Utility functions for dumping Portaudio devices info
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PORTAUDIO_DEVICES_DUMP_H
#define SUSHI_PORTAUDIO_DEVICES_DUMP_H

#include "portaudio_devices_dump.h"
#include "portaudio_frontend.h"
#include "rapidjson/rapidjson.h"

#include "logging.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("portaudio");

/**
 * @brief Retrieve Portaudio's registered devices information.
 *        Can be queried before instantiating an actual PortaudioFrontend
 *
 * @return Device information list in JSON format
 */
rapidjson::Document generate_portaudio_devices_info_document()
{
    PortAudioFrontend frontend{nullptr};

    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    auto n_devs = frontend.devices_count();
    if ( !n_devs.has_value() || (n_devs.value() <= 0) )
    {
        SUSHI_LOG_ERROR("No Portaudio devices found");
        return document;
    }

    rapidjson::Value pa_devices(rapidjson::kObjectType);
    rapidjson::Value devices(rapidjson::kArrayType);
    for (int i = 0; i < n_devs.value(); i++)
    {
        auto devinfo = frontend.device_info(i);
        if (!devinfo.has_value())
        {
            SUSHI_LOG_ERROR("Could not retrieve device info for Portaudio device with idx: {}", i);
            continue;
        }
        auto di = devinfo.value();

        rapidjson::Value device_obj(rapidjson::kObjectType);
        device_obj.AddMember(rapidjson::Value("name", allocator).Move(),
                             rapidjson::Value(di.name.c_str(), allocator).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("inputs", allocator).Move(),
                             rapidjson::Value(di.inputs).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("outputs", allocator).Move(),
                             rapidjson::Value(di.outputs).Move(), allocator);
        devices.PushBack(device_obj.Move(), allocator);
    }
    pa_devices.AddMember(rapidjson::Value("devices", allocator).Move(), devices.Move(), allocator);

    auto default_input = frontend.default_input_device();
    if (! default_input.has_value() )
    {
        SUSHI_LOG_ERROR("Could not retrieve Portaudio default input device");
    }
    else
    {
        pa_devices.AddMember(rapidjson::Value("default_input_device", allocator).Move(),
                             rapidjson::Value(default_input.value()).Move(), allocator);
    }
    auto default_output = frontend.default_output_device();
    if (! default_output.has_value() )
    {
        SUSHI_LOG_ERROR("Could not retrieve Portaudio default output device");
    }
    else
    {
        pa_devices.AddMember(rapidjson::Value("default_output_device", allocator).Move(),
                             rapidjson::Value(default_output.value()).Move(), allocator);
    }

    document.AddMember(rapidjson::Value("portaudio_devices", allocator), pa_devices.Move(), allocator);

    return document;
}

} // end namespace audio_frontend
} // end namespace sushi

#endif // SUSHI_PORTAUDIO_DEVICES_DUMP_H

