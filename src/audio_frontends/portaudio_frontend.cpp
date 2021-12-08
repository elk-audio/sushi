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
 * @brief Realtime audio frontend for PortAudio
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_PORTAUDIO

#include "logging.h"
#include "portaudio_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("PortAudio");

AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto portaudio_config = static_cast<PortAudioConfiguration*>(config);
    SUSHI_LOG_INFO("Initializing port audio with device id {}", portaudio_config->device_id.value_or(-1));
    return AudioFrontendStatus::OK;
}

void PortAudioFrontend::cleanup()
{
    _engine->enable_realtime(false);
}

void PortAudioFrontend::run()
{
    _engine->enable_realtime(true);
}

}; // end namespace audio_frontend
}; // end namespace sushi
#endif
#ifndef SUSHI_BUILD_WITH_PORTAUDIO
#include "audio_frontends/portaudio_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
SUSHI_GET_LOGGER;
PortAudioFrontend::PortAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{}
AudioFrontendStatus PortAudioFrontend::init(BaseAudioFrontendConfiguration*)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    SUSHI_LOG_ERROR("Sushi was not built with PortAudio support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}}}
#endif
