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
 * @brief Audio frontend base classes implementations
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "base_audio_frontend.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("audio_frontend");

namespace sushi {
namespace audio_frontend {

AudioFrontendStatus BaseAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    _config = config;
    try
    {
        _pause_notify = twine::RtConditionVariable::create_rt_condition_variable();
    }
    catch(const std::exception& e)
    {
        SUSHI_LOG_ERROR("Failed to instantiate RtConditionVariable ({})", e.what());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
};

}; // end namespace audio_frontend
}; // end namespace sushi