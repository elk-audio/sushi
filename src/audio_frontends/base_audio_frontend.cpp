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
 * @brief Audio frontend base classes implementations
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"

#include "base_audio_frontend.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("audio_frontend");

namespace sushi::internal::audio_frontend {

AudioFrontendStatus BaseAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    _config = config;
    try
    {
        _pause_notify = twine::RtConditionVariable::create_rt_condition_variable();
    }
    catch (const std::exception& e)
    {
        ELKLOG_LOG_ERROR("Failed to instantiate RtConditionVariable ({})", e.what());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

void BaseAudioFrontend::pause(bool paused)
{
    /* This default implementation is for realtime frontends that must remember to call
     * _pause_notify->notify() in the audio callback when un-pausing */
    assert(twine::is_current_thread_realtime() == false);
    bool running = !_pause_manager.bypassed();
    _pause_manager.set_bypass(paused, _engine->sample_rate());

    // If pausing, return when engine has ramped down.
    if (paused && running)
    {
        _pause_notified = false;
        _pause_notify->wait();
        _engine->enable_realtime(false);
    }
    else
    {
        _engine->enable_realtime(paused);
    }
}

} // end namespace sushi::internal::audio_frontend