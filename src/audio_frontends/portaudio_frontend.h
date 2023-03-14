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
#include <optional>

// TODO: Keep an eye on these deprecated declarations and update when they are fixed.
// There is an open issue on github at the time of writing about C11 which would fix this.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <portaudio.h>
#pragma GCC diagnostic pop

#include "base_audio_frontend.h"

namespace sushi {
namespace audio_frontend {

struct PortaudioDeviceInfo
{
    std::string name;
    int inputs;
    int outputs;
};

struct PortAudioFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    PortAudioFrontendConfiguration(std::optional<int> input_device_id,
                                   std::optional<int> output_device_id,
                                   float suggested_input_latency,
                                   float suggested_output_latency,
                                   int cv_inputs,
                                   int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            input_device_id(input_device_id),
            output_device_id(output_device_id),
            suggested_input_latency(suggested_input_latency),
            suggested_output_latency(suggested_output_latency)
    {}

    virtual ~PortAudioFrontendConfiguration() = default;

    std::optional<int> input_device_id;
    std::optional<int> output_device_id;
    float suggested_input_latency{0.0f};
    float suggested_output_latency{0.0f};
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

    /**
     * @brief Get the n. of available devices.
     *        Can be called before init()
     *
     * @return Total number of devices as reported by Portaudio, or std::nullopt if there was an error
     */
    std::optional<int> devices_count();

    /**
     * @brief Query a device basic properties
     *
     * @param device_idx Device index in [0..devices_count()-1]
     *
     * @return Struct with device name, n. of input channels, n. of output channels.
     *         std::nullopt is returned if there was an error
     */
    std::optional<PortaudioDeviceInfo> device_info(int device_idx);

    /**
     * @brief Query the default input device
     *
     * @return Index in [0..devices_count()-1], or std::nullopt if there was an error
     */
    std::optional<int> default_input_device();

    /**
     * @brief Query the default output device
     *
     * @return Index in [0..devices_count()-1], or std::nullopt if there was an error
     */
    std::optional<int> default_output_device();

private:
    /**
     * @brief Initialize PortAudio engine, and cache the result to avoid multiple initializations
     *
     * @return OK or AUDIO_HW_ERROR
     */
    AudioFrontendStatus _initialize_portaudio();

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

    void _copy_interleaved_audio(const float* input);

    void _output_interleaved_audio(float* output);

    std::array<float, MAX_ENGINE_CV_IO_PORTS> _cv_output_his{0};
    int _num_total_input_channels{0};
    int _num_total_output_channels{0};
    int _audio_input_channels{0};
    int _audio_output_channels{0};
    int _cv_input_channels{0};
    int _cv_output_channels{0};

    bool _pa_initialized{false};
    PaStream* _stream{nullptr};

    // This is convenient mostly for mock testing, where checking for nullptr will not work
    bool _stream_initialized {false};
    const PaDeviceInfo* _input_device_info;
    const PaDeviceInfo* _output_device_info;

    ChunkSampleBuffer _in_buffer{MAX_FRONTEND_CHANNELS};
    ChunkSampleBuffer _out_buffer{MAX_FRONTEND_CHANNELS};

    Time _start_time;
    PaTime _time_offset;
    int64_t _processed_sample_count{0};

    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;
};

} // end namespace audio_frontend
} // end namespace sushi

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
    PortAudioFrontendConfiguration(std::optional<int>, std::optional<int>, float, float, int, int) : BaseAudioFrontendConfiguration(0, 0) {}
};

struct PortaudioDeviceInfo
{
    std::string name;
    int inputs;
    int outputs;
};

class PortAudioFrontend : public BaseAudioFrontend
{
public:
    PortAudioFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override;
    void cleanup() override {}
    void run() override {}
    void pause([[maybe_unused]] bool enabled) override {}
    std::optional<int> devices_count() { return 0; }
    std::optional<PortaudioDeviceInfo> device_info(int /*device_idx*/) { return PortaudioDeviceInfo(); }
    std::optional<int> default_input_device() { return 0; }
    std::optional<int> default_output_device() { return 0; }
};
}; // end namespace audio_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_PORTAUDIO_FRONTEND_H

