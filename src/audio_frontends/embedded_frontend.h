/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Embedded frontend to process audio from a callback through a host application.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_EMBEDDED_FRONTEND_H
#define SUSHI_EMBEDDED_FRONTEND_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>

#include <sndfile.h>

#include "base_audio_frontend.h"
#include "library/rt_event.h"

namespace sushi {

namespace audio_frontend {

using TimePoint = std::chrono::nanoseconds;

// TODO:
//   Hard-coding the number of channels for now.
constexpr int EMBEDDED_FRONTEND_CHANNELS = 2;

struct EmbeddedFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    // TODO: Debug mode switches?
    EmbeddedFrontendConfiguration(int cv_inputs,
                                  int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs)
    {}

    virtual ~EmbeddedFrontendConfiguration() = default;
};

class EmbeddedFrontend : public BaseAudioFrontend
{
public:
    EmbeddedFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~EmbeddedFrontend()
    {
        cleanup();
    }

    /**
     * @brief Initialize frontend with the given configuration.
     *        If anything can go wrong during initialization, partially allocated
     *        resources should be freed by calling cleanup().
     *
     * @param config Should be an object of the proper derived configuration class.
     * @return AudioFrontendInitStatus::OK in case of success,
     *         or different error code otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Free resources allocated during init. stops the frontend if currently running.
     */
    void cleanup() override;

    /**
     * @brief Run engine main loop.
     */
    void run() override;

    // TODO: Currently the buffers are pointers as that's what's used and expected inside sushi.
    //  - I'd rather they are references, otherwise the API communicates the
    //  assumption they CAN be nullptr.

    /**
     * Method to invoke from the host's audio callback.
     * @param input buffer
     * @param output buffer
     * @param channel_count
     * @param sample_count
     */
     void process_audio(ChunkSampleBuffer* in_buffer, // Not const, because process_chunk expects this as a raw pointer.
                        ChunkSampleBuffer* out_buffer,
                       int channel_count,
                       int sample_count);

private:
    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;

    TimePoint _start_time;
};

} // end namespace audio_frontend

} // end namespace sushi

#endif //SUSHI_EMBEDDED_FRONTEND_H
