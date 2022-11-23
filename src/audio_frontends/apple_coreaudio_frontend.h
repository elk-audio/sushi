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
* @brief Realtime audio frontend for Apple CoreAudio
* @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_FRONTEND_H
#define SUSHI_APPLE_COREAUDIO_FRONTEND_H

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include "base_audio_frontend.h"
#include "json_utils.h"

namespace sushi::audio_frontend {

struct AppleCoreAudioFrontendConfiguration : public BaseAudioFrontendConfiguration {
    AppleCoreAudioFrontendConfiguration(std::optional<int> input_device_id,
                                        std::optional<int> output_device_id,
                                        int cv_inputs,
                                        int cv_outputs) : BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
                                                          input_device_id(input_device_id),
                                                          output_device_id(output_device_id)
    {}

    ~AppleCoreAudioFrontendConfiguration() override = default;

    std::optional<int> input_device_id;
    std::optional<int> output_device_id;
};

class AppleCoreAudioFrontend : public BaseAudioFrontend
{
public:
    explicit AppleCoreAudioFrontend(engine::BaseEngine* engine);

    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;
    void cleanup() override;
    void run() override;
    void pause(bool enabled) override;

    static rapidjson::Document generate_devices_info_document();
};

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
/* If Apple CoreAudio is disabled in the build config, the AppleCoreAudio frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */

#include "base_audio_frontend.h"

namespace sushi::audio_frontend {

struct AppleCoreAudioFrontendConfiguration : public BaseAudioFrontendConfiguration {
    AppleCoreAudioFrontendConfiguration(std::optional<int>, std::optional<int>, float, float, int, int) : BaseAudioFrontendConfiguration(0, 0) {}
};

class AppleCoreAudioFrontend : public BaseAudioFrontend
{
public:
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    void cleanup() override{};
    void run() override{};
};

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO

#endif// SUSHI_APPLE_COREAUDIO_FRONTEND_H
