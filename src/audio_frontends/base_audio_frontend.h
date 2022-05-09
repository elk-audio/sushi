/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Base classes for audio frontends
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#ifndef SUSHI_BASE_AUDIO_FRONTEND_H
#define SUSHI_BASE_AUDIO_FRONTEND_H

#include "engine/base_engine.h"

namespace sushi {

namespace audio_frontend {

constexpr int MAX_FRONTEND_CHANNELS = 8;

/**
 * @brief Error codes returned from init()
 */
enum class AudioFrontendStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_INPUT_FILE,
    INVALID_OUTPUT_FILE,
    INVALID_SEQUENCER_DATA,
    INVALID_CHUNK_SIZE,
    AUDIO_HW_ERROR
};

/**
 * @brief Dummy base class to hold frontend configurations
 */
struct BaseAudioFrontendConfiguration
{
    BaseAudioFrontendConfiguration(int cv_inputs, int cv_outputs) : cv_inputs{cv_inputs},
                                                                    cv_outputs{cv_outputs} {}

    virtual ~BaseAudioFrontendConfiguration() = default;
    int cv_inputs;
    int cv_outputs;
};

/**
 * @brief Base class for Engine Frontends.
 */
class BaseAudioFrontend
{
public:
    BaseAudioFrontend(engine::BaseEngine* engine) : _engine(engine) {}

    virtual ~BaseAudioFrontend() = default;

    /**
     * @brief Initialize frontend with the given configuration.
     *        If anything can go wrong during initialization, partially allocated
     *        resources should be freed by calling cleanup().
     *
     * @param config Should be an object of the proper derived configuration class.
     * @return AudioFrontendInitStatus::OK in case of success,
     *         or different error code otherwise.
     */
    virtual AudioFrontendStatus init(BaseAudioFrontendConfiguration* config);

    /**
     * @brief Free resources allocated during init. stops the frontend if currently running.
     */
    virtual void cleanup() = 0;

    /**
     * @brief Run engine main loop.
     */
    virtual void run() = 0;

    /**
     * @brief Pause a running frontend. If paused, any threads set up are still running and audio
     *        data consumed, but the audio engine is not called and all audio outputs are silenced.
     *        When toggling pause, the audio will be quickly ramped down and the function will block
     *        until the change has taken effect.
     * @param enabled If true enables pause, of false disables pause and calls the audio engine again
     */
     virtual void pause(bool enabled);

protected:
    BaseAudioFrontendConfiguration* _config;
    engine::BaseEngine* _engine;

    BypassManager _pause_manager;
    std::unique_ptr<twine::RtConditionVariable> _pause_notify;
    std::atomic_bool _pause_notified{false};
};


} // end namespace audio_frontend

} // end namespace sushi

#endif //SUSHI_BASE_AUDIO_FRONTEND_H
