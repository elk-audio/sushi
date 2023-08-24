/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Utility functions for dumping CoreAudio devices info
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"
#include "rapidjson/rapidjson.h"

#include "sushi/coreaudio_devices_dump.h"

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include "apple_coreaudio_frontend.h"

#include "audio_frontends/apple_coreaudio/apple_coreaudio_system_object.h"

namespace sushi {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("coreaudio");

rapidjson::Document generate_coreaudio_devices_info_document()
{
    internal::audio_frontend::AppleCoreAudioFrontend frontend{nullptr};

    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

    auto audio_devices = apple_coreaudio::AudioSystemObject::get_audio_devices();
    if (audio_devices.empty())
    {
        ELKLOG_LOG_ERROR("No Apple CoreAudio devices found");
        return document;
    }

    rapidjson::Value ca_devices(rapidjson::kObjectType);
    rapidjson::Value devices(rapidjson::kArrayType);
    for (auto& device : audio_devices)
    {
        rapidjson::Value device_obj(rapidjson::kObjectType);
        device_obj.AddMember(rapidjson::Value("name", allocator).Move(),
                             rapidjson::Value(device.name().c_str(), allocator).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("uid", allocator).Move(),
                             rapidjson::Value(device.uid().c_str(), allocator).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("inputs", allocator).Move(),
                             rapidjson::Value(device.num_channels(true)).Move(), allocator);
        device_obj.AddMember(rapidjson::Value("outputs", allocator).Move(),
                             rapidjson::Value(device.num_channels(false)).Move(), allocator);

        // Add available sample rates as array
        rapidjson::Value sample_rates(rapidjson::kArrayType);
        for (auto& rate : device.available_nominal_sample_rates())
        {
            sample_rates.PushBack(static_cast<uint64_t>(rate), allocator);
        }

        device_obj.AddMember(rapidjson::Value("available_sample_rates", allocator).Move(), sample_rates, allocator);

        // Add available buffer sizes as object with min and max values
        rapidjson::Value buffer_frame_size_range(rapidjson::kObjectType);

        auto buffer_sizes = device.available_buffer_sizes();
        buffer_frame_size_range.AddMember(rapidjson::Value("min", allocator).Move(), buffer_sizes.mMinimum, allocator);
        buffer_frame_size_range.AddMember(rapidjson::Value("max", allocator).Move(), buffer_sizes.mMaximum, allocator);

        device_obj.AddMember(rapidjson::Value("buffer_frame_size_range", allocator).Move(), buffer_frame_size_range.Move(), allocator);

        devices.PushBack(device_obj.Move(), allocator);
    }
    ca_devices.AddMember(rapidjson::Value("devices", allocator).Move(), devices.Move(), allocator);

    auto add_default_device_index = [&audio_devices, &ca_devices, &allocator](bool for_input) {
        auto default_audio_device_object_id = apple_coreaudio::AudioSystemObject::get_default_device_id(for_input);

        for (auto it = audio_devices.begin(); it != audio_devices.end(); it++)
        {
            if (it->get_audio_object_id() == default_audio_device_object_id)
            {
                ca_devices.AddMember(rapidjson::Value(for_input ? "default_input_device" : "default_output_device", allocator).Move(),
                                     rapidjson::Value(static_cast<uint64_t>(std::distance(audio_devices.begin(), it))).Move(), allocator);
                return;
            }
        }

        ELKLOG_LOG_ERROR("Could not retrieve Apple CoreAudio default {} device", for_input ? "input" : "output");
    };

    add_default_device_index(true);
    add_default_device_index(false);

    document.AddMember(rapidjson::Value("apple_coreaudio_devices", allocator), ca_devices.Move(), allocator);

    return document;
}

} // end namespace sushi

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO

namespace sushi {

rapidjson::Document generate_coreaudio_devices_info_document()
{
    rapidjson::Document document;

    return document;
}

} // end namespace sushi


#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO
