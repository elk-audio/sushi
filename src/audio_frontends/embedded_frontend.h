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
    EmbeddedFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
    {

    }

    virtual ~EmbeddedFrontend()
    {
        cleanup();
    }

    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    void cleanup() override;

    void run() override;

    void process_audio(const float** array_of_read_pointers,
                       float** array_of_write_pointers,
                       int channel_count,
                       int sample_count);

private:
    ChunkSampleBuffer _in_buffer {MAX_FRONTEND_CHANNELS};
    ChunkSampleBuffer _out_buffer {MAX_FRONTEND_CHANNELS};

    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;

    TimePoint _start_time;
};

} // end namespace audio_frontend

} // end namespace sushi

#endif //SUSHI_EMBEDDED_FRONTEND_H
