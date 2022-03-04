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
// There is an open issue on github at the time of writing about C11 which would fix this.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <portaudio.h>
#pragma GCC diagnostic pop

#include "base_audio_frontend.h"

namespace sushi {
namespace audio_frontend {

struct PortAudioFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    PortAudioFrontendConfiguration(std::optional<int> input_device_id,
                           std::optional<int> output_device_id,
                           int cv_inputs,
                           int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            input_device_id(input_device_id),
            output_device_id(output_device_id)
    {}

    virtual ~PortAudioFrontendConfiguration() = default;

    std::optional<int> input_device_id;
    std::optional<int> output_device_id;
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
     * @brief The realtime process callback given to Port Audio which will be
     *        called for every processing chunk.
     *
     * @param input pointer to the interleaved input data. Needs to be cast to the correct sample format
     * @param output pointer to the interleaved output data. Needs to be cast to the correct sample format
     * @param frame_count number of frames to process
     * @param time_info timing information for the buffers passed to the stream callback
     * @param status_flags is set if under or overflow has occurred
     * @param user_data  pointer to the PortAudioFrontend instance
     * @return int
     */
    static int rt_process_callback(const void* input,
                                   void* output,
                                   unsigned long frame_count,
                                   const PaStreamCallbackTimeInfo* time_info,
                                   PaStreamCallbackFlags status_flags,
                                   void* user_data)
    {
        return static_cast<PortAudioFrontend*>(user_data)->_internal_process_callback(input,
                                                                                     output,
                                                                                     frame_count,
                                                                                     time_info,
                                                                                     status_flags);
    }

    /**
     * @brief Initialize the frontend and setup PortAudio client.
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
    AudioFrontendStatus _configure_audio_channels(const PortAudioFrontendConfiguration* config);

    /**
     * @brief Configure the samplerate to use. First tests if the value of the given
     * samplerate is compatible with the input and output parameters. If not it will test
     * the default samplerate of the input and output device respectively. The value of
     * the samplerate parameter will be set to the samplerate that was found to work.
     *
     * @param input_parameters input configuration to test against the samplerate
     * @param output_parameters output configuration to test against the samplerate
     * @param samplerate pointer to a float containing the first samplerate to test
     * @return true if a samplerate was found
     * @return false if no samplerate was found
     */
    bool _configure_samplerate(const PaStreamParameters* input_parameters,
                               const PaStreamParameters* output_parameters,
                               double* samplerate);

    int _internal_process_callback(const void* input,
                                   void* output,
                                   unsigned long frame_count,
                                   const PaStreamCallbackTimeInfo* time_info,
                                   PaStreamCallbackFlags status_flags);

    std::array<float, MAX_ENGINE_CV_IO_PORTS> _cv_output_his{0};
    int _num_total_input_channels{0};
    int _num_total_output_channels{0};
    int _audio_input_channels{0};
    int _audio_output_channels{0};
    int _cv_input_channels{0};
    int _cv_output_channels{0};

    PaStream* _stream;
    const PaDeviceInfo* _input_device_info;
    const PaDeviceInfo* _output_device_info;

    Time _start_time;
    PaTime _time_offset;
    int64_t _processed_sample_count{0};

    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;
};

}; // end namespace audio_frontend
}; // end namespace sushi

#endif //SUSHI_BUILD_WITH_PORTAUDIO
#ifndef SUSHI_BUILD_WITH_PORTAUDIO
/* If PortAudio is disabled in the build config, the PortAudio frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */
#include <string>
#include "base_audio_frontend.h"
#include "engine/midi_dispatcher.h"
namespace sushi {
namespace audio_frontend {
struct PortAudioFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    PortAudioFrontendConfiguration(std::optional<int> input_device_id,
                                   std::optional<int> output_device_id,
                                   int cv_inputs,
                                   int cv_outputs) : BaseAudioFrontendConfiguration(0, 0) {}
};

class PortAudioFrontend : public BaseAudioFrontend
{
public:
    PortAudioFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override;
    void cleanup() override {}
    void run() override {}
};
}; // end namespace audio_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_PORTAUDIO_FRONTEND_H
