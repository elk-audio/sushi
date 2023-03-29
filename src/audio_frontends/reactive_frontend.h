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
 * @brief Reactive frontend to process audio from a callback through a host application.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_REACTIVE_FRONTEND_H
#define SUSHI_REACTIVE_FRONTEND_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>

#include <sndfile.h>

#include "base_audio_frontend.h"
#include "library/rt_event.h"

namespace sushi::internal::audio_frontend {

// TODO:
//   Hard-coding the number of channels for now.
constexpr int REACTIVE_FRONTEND_CHANNELS = 2;

struct ReactiveFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    ReactiveFrontendConfiguration(int cv_inputs,
                                  int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs)
    {}

    ~ReactiveFrontendConfiguration() override = default;
};

class ReactiveFrontend : public BaseAudioFrontend
{
public:
    ReactiveFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~ReactiveFrontend()
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

    /**
     * @brief Method to invoke from the host's audio callback.
     * @param channel_count
     * @param total_sample_count since start (timestamp)
     * @param timestamp
     */
     void process_audio(int channel_count,
                        int total_sample_count,
                        Time timestamp);

     ChunkSampleBuffer& in_buffer();
     ChunkSampleBuffer& out_buffer();

private:
    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;

    ChunkSampleBuffer _in_buffer;
    ChunkSampleBuffer _out_buffer;

    Time _start_time;
};

} // end namespace sushi::internal::audio_frontend

#endif // SUSHI_REACTIVE_FRONTEND_H
