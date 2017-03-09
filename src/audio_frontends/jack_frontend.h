/**
 * @brief Realtime Jackaudio frontend
 *
 */

#ifndef SUSHI_JACK_FRONTEND_H
#define SUSHI_JACK_FRONTEND_H


#include "base_audio_frontend.h"
#include "library/plugin_events.h"
#include "library/event_fifo.h"
#include "control_frontends/osc_frontend.h"

#include <string>
#include <tuple>
#include <vector>

#include <json/json.h>
#include <jack/jack.h>

namespace sushi {

namespace audio_frontend {

constexpr int MAX_FRONTEND_CHANNELS = 2;
constexpr int MAX_EVENTS_PER_CHUNK = 100;

struct JackFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    JackFrontendConfiguration(const std::string client_name,
                              const std::string server_name,
                              bool autoconnect_ports) :
            client_name(client_name),
            server_name(server_name),
            autoconnect_ports(autoconnect_ports)
    {}

    virtual ~JackFrontendConfiguration()
    {}

    std::string client_name;
    std::string server_name;
    bool autoconnect_ports;
};

class JackFrontend : public BaseAudioFrontend
{
public:
    JackFrontend(engine::BaseEngine* engine) :
            BaseAudioFrontend(engine)
    {}

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
    AudioFrontendStatus setup_client(const std::string client_name, const std::string server_name);

    AudioFrontendStatus setup_ports();
    /* Call after activation to connect the frontend ports to system ports */
    AudioFrontendStatus connect_ports();

    /* Internal process callback function */
    int internal_process_callback(jack_nframes_t nframes);

    void process_events();
    void process_midi(jack_nframes_t no_frames);
    void process_audio(jack_nframes_t no_frames);

    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _output_ports;
    std::array<jack_port_t*, MAX_FRONTEND_CHANNELS> _input_ports;
    jack_port_t* _midi_port;
    jack_client_t* _client{nullptr};
    bool _autoconnect_ports{false};

    SampleBuffer<AUDIO_CHUNK_SIZE> _in_buffer{MAX_FRONTEND_CHANNELS};
    SampleBuffer<AUDIO_CHUNK_SIZE> _out_buffer{MAX_FRONTEND_CHANNELS};

    EventFifo _event_queue;

    std::unique_ptr<control_frontend::OSCFrontend> _osc_control;
};

}; // end namespace jack_frontend

}; // end namespace sushi

#endif //SUSHI_JACK_FRONTEND_H
