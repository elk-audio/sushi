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
 * @brief Realtime audio frontend for Jack Audio
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_JACK_FRONTEND_H
#define SUSHI_JACK_FRONTEND_H
#ifdef SUSHI_BUILD_WITH_JACK

#include <string>
#include <memory>

#include <jack/jack.h>
#include "twine/twine.h"

#include "base_audio_frontend.h"

namespace sushi {
namespace audio_frontend {

struct JackFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    JackFrontendConfiguration(const std::string& client_name,
                              const std::string& server_name,
                              bool autoconnect_ports,
                              int cv_inputs,
                              int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            client_name(client_name),
            server_name(server_name),
            autoconnect_ports(autoconnect_ports)
    {}

    virtual ~JackFrontendConfiguration() = default;

    std::string client_name;
    std::string server_name;
    bool autoconnect_ports;
};

class JackFrontend : public BaseAudioFrontend
{
public:
    JackFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~JackFrontend()
    {
        cleanup();
    }

    /**
     * @brief The realtime process callback given to jack and which will be
     *        called for every processing chunk.
     * @param nframes Number of frames in this processing chunk.
     * @param arg In this case is a pointer to the JackFrontend instance.
     * @return
     */
    static int rt_process_callback(jack_nframes_t nframes, void *arg)
    {
        return static_cast<JackFrontend*>(arg)->internal_process_callback(nframes);
    }

    /**
     * @brief Callback for sample rate changes
     * @param nframes New samplerate in Samples per second
     * @param arg Pointer to the JackFrontend instance.
     * @return
     */
    static int samplerate_callback(jack_nframes_t nframes, void *arg)
    {
        return static_cast<JackFrontend*>(arg)->internal_samplerate_callback(nframes);
    }

    static void latency_callback(jack_latency_callback_mode_t mode, void *arg)
    {
        return static_cast<JackFrontend*>(arg)->internal_latency_callback(mode);
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
    /* Set up the jack client and associated ports */
    AudioFrontendStatus setup_client(const std::string& client_name, const std::string& server_name);
    AudioFrontendStatus setup_sample_rate();
    AudioFrontendStatus setup_ports();
    AudioFrontendStatus setup_cv_ports();
    /* Call after activation to connect the frontend ports to system ports */
    AudioFrontendStatus connect_ports();

    /* Internal process callback function */
    int internal_process_callback(jack_nframes_t framecount);
    int internal_samplerate_callback(jack_nframes_t sample_rate);
    void internal_latency_callback(jack_latency_callback_mode_t mode);

    void process_audio(jack_nframes_t start_frame, jack_nframes_t framecount, Time timestamp, int64_t samplecount);

    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _input_ports;
    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _output_ports;
    std::array<jack_port_t*, MAX_ENGINE_CV_IO_PORTS> _cv_input_ports;
    std::array<jack_port_t*, MAX_ENGINE_CV_IO_PORTS> _cv_output_ports;
    std::array<float, MAX_ENGINE_CV_IO_PORTS> _cv_output_hist{0};
    int _no_cv_input_ports;
    int _no_cv_output_ports;

    jack_client_t* _client{nullptr};
    jack_nframes_t _sample_rate;
    jack_nframes_t _start_frame{0};
    bool _autoconnect_ports{false};

    SampleBuffer<AUDIO_CHUNK_SIZE> _in_buffer{MAX_FRONTEND_CHANNELS};
    SampleBuffer<AUDIO_CHUNK_SIZE> _out_buffer{MAX_FRONTEND_CHANNELS};
    engine::ControlBuffer          _in_controls;
    engine::ControlBuffer          _out_controls;
};

} // end namespace jack_frontend
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_JACK
#ifndef SUSHI_BUILD_WITH_JACK
/* If Jack is disabled in the build config, the jack frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */
#include <string>
#include "base_audio_frontend.h"
#include "engine/midi_dispatcher.h"
namespace sushi {
namespace audio_frontend {
struct JackFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    JackFrontendConfiguration(const std::string&,
                              const std::string&,
                              bool, int, int) : BaseAudioFrontendConfiguration(0, 0) {}
};

class JackFrontend : public BaseAudioFrontend
{
public:
    JackFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override;
    void cleanup() override {}
    void run() override {}
    void pause([[maybe_unused]] bool enabled) override {}
};
}; // end namespace jack_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_JACK_FRONTEND_H
