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

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include "apple_coreaudio_frontend.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("AppleCoreAudio");

namespace sushi::audio_frontend {

AppleCoreAudioFrontend::AppleCoreAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{
}

AudioFrontendStatus AppleCoreAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    return BaseAudioFrontend::init(config);
}

void AppleCoreAudioFrontend::cleanup()
{
}

void AppleCoreAudioFrontend::run()
{
}

void AppleCoreAudioFrontend::pause(bool enabled)
{
    BaseAudioFrontend::pause(enabled);
}

rapidjson::Document AppleCoreAudioFrontend::generate_devices_info_document()
{
    rapidjson::Document document;
    document.SetObject();
    // TODO: Implement
    return document;
}

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
#include "apple_coreaudio_frontend.h"
#include "logging.h"

SUSHI_GET_LOGGER;

namespace sushi::audio_frontend {

sushi::audio_frontend::AudioFrontendStatus sushi::audio_frontend::AppleCoreAudioFrontend::init(
        [[maybe_unused]] sushi::audio_frontend::BaseAudioFrontendConfiguration* config)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    SUSHI_LOG_ERROR("Sushi was not built with CoreAudio support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}

}// namespace sushi::audio_frontend

#endif// SUSHI_BUILD_WITH_APPLE_COREAUDIO
