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

constexpr float XRUN_LIMIT_FACTOR = 1.8f;

AudioFrontendStatus BaseAudioFrontend::init(BaseAudioFrontendConfiguration* config)
{
    _config = config;
    try
    {
        _pause_notify = twine::RtConditionVariable::create_rt_condition_variable();
    }
    catch ([[maybe_unused]] const std::exception& e)
    {
        ELKLOG_LOG_ERROR("Failed to instantiate RtConditionVariable ({})", e.what());
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

void BaseAudioFrontend::pause(bool paused)
{
    /* This default implementation is for realtime frontends that must remember to call
     * _handle_resume() and _handle_pause() in the audio callback */
    assert(twine::is_current_thread_realtime() == false);
    bool running = !_pause_manager.bypassed();
    _pause_manager.set_bypass(paused, _engine->sample_rate());

    // If pausing, return when engine has ramped down.
    if (paused && running)
    {
        _pause_notified = false;
        _pause_notify->wait();
        _engine->enable_realtime(false);
        _resume_notified = false;
    }
    else
    {
        if (!paused and !running)
        {
            _engine->enable_realtime(true);
        }
    }
}

std::pair<bool, Time> BaseAudioFrontend::_test_for_xruns(Time current_time, int current_samples)
{
    auto delta_time = current_time - _last_process_time;
    bool first = _last_process_time == Time::zero();
    auto limit = Time(static_cast<int>(current_samples * _inv_sample_rate * XRUN_LIMIT_FACTOR * Time(std::chrono::seconds(1)).count()));
    _last_process_time = current_time;

    if (!first && (delta_time != Time::zero()) && (std::abs(delta_time.count()) > limit.count()))
    {
        return {true, delta_time};
    }
    return {false, Time(0)};
}

void BaseAudioFrontend::_handle_resume(Time current_time, int current_samples)
{
    if (!_resume_notified && _pause_manager.should_process())
    {
        _resume_notified = true;
        _engine->notify_interrupted_audio(current_time - _pause_start);
    }
    else
    {
        auto [xrun, delta_time] = _test_for_xruns(current_time, current_samples);
        if (xrun)
        {
            _engine->notify_interrupted_audio(delta_time);
        }
    }
}

void BaseAudioFrontend::_handle_pause(Time current_time)
{
    if (_pause_notified == false && _pause_manager.should_process() == false)
    {
        _pause_notify->notify();
        _pause_notified = true;
        _pause_start = current_time;
    }
}

void BaseAudioFrontend::_set_engine_sample_rate(float sample_rate)
{
    if (_engine->sample_rate() != sample_rate)
    {
        _engine->set_sample_rate(sample_rate);
    }
    _sample_rate = sample_rate;
    _inv_sample_rate = 1.0f / sample_rate;
}

} // end namespace sushi::internal::audio_frontend