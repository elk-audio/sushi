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

#ifndef SUSHI_PORTAUDIO_FRONTEND_H
#define SUSHI_PORTAUDIO_FRONTEND_H
#ifdef SUSHI_BUILD_WITH_PORTAUDIO

#include <string>
#include <memory>

// TODO: Keep an eye on these deprecated declarations and update when they are fixed.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <portaudio.h>
#pragma GCC diagnostic pop

#include "base_audio_frontend.h"

namespace sushi {
namespace audio_frontend {

struct PortAudioConfiguration : public BaseAudioFrontendConfiguration
{
    PortAudioConfiguration(int device_id,
                           int cv_inputs,
                           int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            device_id(device_id)
    {}

    virtual ~PortAudioConfiguration() = default;

    int device_id;
};

class PortAudioFrontend : public BaseAudioFrontend
{
public:
    PortAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~PortAudioFrontend()
    {
        cleanup();
    }

    /**
     * @brief Initialize the frontend and setup Jack client.
     * @param config Configuration struct
     * @return OK on successful initialization, error otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Call to clean up resources and release ports
     */
    void cleanup() override;

    /**
     * @brief Activate the realtime frontend, currently blocking.
     */
    void run() override;

private:
    SampleBuffer<AUDIO_CHUNK_SIZE> _in_buffer{MAX_FRONTEND_CHANNELS};
    SampleBuffer<AUDIO_CHUNK_SIZE> _out_buffer{MAX_FRONTEND_CHANNELS};
    engine::ControlBuffer          _in_controls;
    engine::ControlBuffer          _out_controls;
};

}; // end namespace jack_frontend
}; // end namespace sushi

#endif //SUSHI_BUILD_WITH_PORTAUDIO
#ifndef SUSHI_BUILD_WITH_PORTAUDIO
/* If Jack is disabled in the build config, the jack frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */
#include <string>
#include "base_audio_frontend.h"
#include "engine/midi_dispatcher.h"
namespace sushi {
namespace audio_frontend {
struct PortAudioConfiguration : public BaseAudioFrontendConfiguration
{
    PortAudioConfiguration(int, int, int) : BaseAudioFrontendConfiguration(0, 0) {}
};

class PortAudioFrontend : public BaseAudioFrontend
{
public:
    PortAudioFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override;
    void cleanup() override {}
    void run() override {}
};
}; // end namespace jack_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_PORTAUDIO_FRONTEND_H
