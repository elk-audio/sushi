#ifdef SUSHI_BUILD_WITH_JACK
#include <thread>
#include <deque>
#include <unistd.h>
#include <cmath>

#include <jack/midiport.h>

#include "logging.h"
#include "jack_frontend.h"

namespace sushi {
namespace audio_frontend {

MIND_GET_LOGGER;


AudioFrontendStatus JackFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }
    _osc_control = std::make_unique<control_frontend::OSCFrontend>(&_event_queue, _engine);
    _osc_control->connect_all();
    auto jack_config = static_cast<JackFrontendConfiguration*>(_config);
    _autoconnect_ports = jack_config->autoconnect_ports;
    return setup_client(jack_config->client_name, jack_config->server_name);
}


void JackFrontend::cleanup()
{
    if (_client)
    {
        jack_client_close(_client);
        _client = nullptr;
    }
}


void JackFrontend::run()
{
    _engine->enable_realtime(true);
    int status = jack_activate(_client);
    if (status != 0)
    {
        MIND_LOG_ERROR("Failed to activate Jack client, error {}.", status);
    }
    if (_autoconnect_ports)
    {
        connect_ports();
    }
    _osc_control->run();
    sleep(1000);
}


AudioFrontendStatus JackFrontend::setup_client(const std::string client_name,
                                               const std::string server_name)
{
    jack_status_t jack_status;
    jack_options_t options = JackNullOption;
    if (!server_name.empty())
    {
        MIND_LOG_ERROR("Using option JackServerName");
        options = JackServerName;
    }
    _client = jack_client_open(client_name.c_str(), options, &jack_status, server_name.c_str());
    if (!_client)
    {
        MIND_LOG_ERROR("Failed to open Jack server, error: {}.", jack_status);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    /* Set process callback function */
    int ret = jack_set_process_callback(_client, rt_process_callback, this);
    if (ret != 0)
    {
        MIND_LOG_ERROR("Failed to set Jack callback function, error: {}.", ret);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    auto status = setup_sample_rate();
    if (status != AudioFrontendStatus::OK)
    {
        MIND_LOG_ERROR("Failed to setup sample rate handling");
        return status;
    }
    return setup_ports();
}

AudioFrontendStatus JackFrontend::setup_sample_rate()
{
    _sample_rate = jack_get_sample_rate(_client);
    if (std::lround(_sample_rate) != _engine->sample_rate())
    {
        MIND_LOG_WARNING("Sample rate mismatch between engine ({}) and jack ({})", _engine->sample_rate(), _sample_rate);
        _engine->set_sample_rate(_sample_rate);
    }
    auto status = jack_set_sample_rate_callback(_client, samplerate_callback, this);
    if (status != 0)
    {
        MIND_LOG_WARNING("Setting sample rate callback failed with error {}", status);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

AudioFrontendStatus JackFrontend::setup_ports()
{
    int port_no = 0;
    for (auto& port : _output_ports)
    {
        port = jack_port_register (_client,
                                   std::string("audio_output_" + std::to_string(port_no++)).c_str(),
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsOutput,
                                   0);
        if (!port)
        {
            MIND_LOG_ERROR("Failed to open Jack output port {}.", port_no - 1);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    port_no = 0;
    for (auto& port : _input_ports)
    {
        port = jack_port_register (_client,
                                   std::string("audio_input_" + std::to_string(port_no++)).c_str(),
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsInput,
                                   0);
        if (!port)
        {
            MIND_LOG_ERROR("Failed to open Jack input port {}.", port_no - 1);
            return AudioFrontendStatus::AUDIO_HW_ERROR;
        }
    }
    _midi_port = jack_port_register(_client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    if (!_midi_port)
    {
        MIND_LOG_ERROR("Failed to open Jack Midi input port.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

/*
 * Searches for external ports and tries to autoconnect them with sushis ports.
 */
AudioFrontendStatus JackFrontend::connect_ports()
{
    const char** out_ports = jack_get_ports(_client, nullptr, nullptr, JackPortIsPhysical|JackPortIsInput);
    if (!out_ports)
    {
        MIND_LOG_ERROR("Failed to get ports from Jack.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    for (int id = 0; id < static_cast<int>(_input_ports.size()); ++id)
    {
        if (out_ports[id])
        {
            int ret = jack_connect(_client, jack_port_name(_output_ports[id]), out_ports[id]);
            if (ret != 0)
            {
                MIND_LOG_WARNING("Failed to connect out port {}, error {}.", jack_port_name(_output_ports[id]), id);
            }
        }
    }
    jack_free(out_ports);

    /* Same for input ports */
    const char** in_ports = jack_get_ports(_client, nullptr, nullptr, JackPortIsPhysical|JackPortIsOutput);
    if (!in_ports)
    {
        MIND_LOG_ERROR("Failed to get ports from Jack.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    for (int id = 0; id < static_cast<int>(_input_ports.size()); ++id)
    {
        if (in_ports[id])
        {
            int ret = jack_connect(_client, jack_port_name(_input_ports[id]), in_ports[id]);
            if (ret != 0)
            {
                MIND_LOG_WARNING("Failed to connect port {}, error {}.", jack_port_name(_input_ports[id]), id);
            }
        }
    }
    jack_free(in_ports);

    const char** midi_ports = jack_get_ports(_client, nullptr, JACK_DEFAULT_MIDI_TYPE, JackPortIsPhysical|JackPortIsOutput);
    if (midi_ports)
    {
        int ret = jack_connect(_client, jack_port_name(_midi_port), midi_ports[0]);
        if (ret != 0)
        {
            MIND_LOG_WARNING("Failed to connect Midi port, error {}.", ret);
        }
    }
    jack_free(midi_ports);
    return AudioFrontendStatus::OK;
}


int JackFrontend::internal_process_callback(jack_nframes_t no_frames)
{
    if (no_frames < 64 || no_frames % 64)
    {
        MIND_LOG_WARNING("Chunk size not a multiple of AUDIO_CHUNK_SIZE. Skipping.");
        return 0;
    }
    process_events();
    process_midi(no_frames);
    process_audio(no_frames);
    return 0;
}

int JackFrontend::internal_samplerate_callback(jack_nframes_t sample_rate)
{
    /* It's not fully clear if this is needed since the sample rate can't
     * change without restarting the Jack server. Thought is's hinted that
     * this could be called with a different sample rated than the one
     * requested if the interface doesn't support it. */
    if (_sample_rate != sample_rate)
    {
        MIND_LOG_DEBUG("Received a sample rate change from Jack ({})", sample_rate);
        _engine->set_sample_rate(sample_rate);
        _sample_rate = sample_rate;
    }
    return 0;
}


void inline JackFrontend::process_events()
{
    while (!_event_queue.empty())
    {
        Event event;
        if (_event_queue.pop(event))
        {
            _engine->send_rt_event(event);
        }
    }
}


void inline JackFrontend::process_midi(jack_nframes_t no_frames)
{
    auto* buffer = jack_port_get_buffer(_midi_port, no_frames);
    auto no_events = jack_midi_get_event_count(buffer);
    for (auto i = 0u; i < no_events; ++i)
    {
        jack_midi_event_t midi_event;
        int ret = jack_midi_event_get(&midi_event, buffer, i);
        if (ret == 0)
        {
            _midi_dispatcher->process_midi(0, 0, midi_event.buffer, midi_event.size, true);
        }
    }
}


void inline JackFrontend::process_audio(jack_nframes_t no_frames)
{
    /* Get pointers to audio buffers from ports */
    std::array<const float*, MAX_FRONTEND_CHANNELS> in_data;
    std::array<float*, MAX_FRONTEND_CHANNELS> out_data;

    for (size_t i = 0; i < in_data.size(); ++i)
    {
        in_data[i] = static_cast<float*>(jack_port_get_buffer(_input_ports[i], no_frames));
    }
    for (size_t i = 0; i < in_data.size(); ++i)
    {
        out_data[i] = static_cast<float*>(jack_port_get_buffer(_output_ports[i], no_frames));
    }

    /* And process audio in chunks of size AUDIO_CHUNK_SIZE */
    for (jack_nframes_t frames = 0; frames < no_frames; frames += AUDIO_CHUNK_SIZE)
    {
        for (size_t i = 0; i < _input_ports.size(); ++i)
        {
            std::copy(in_data[i] + frames, in_data[i] + frames + AUDIO_CHUNK_SIZE, _in_buffer.channel(i));
        }
        _out_buffer.clear();
        _engine->process_chunk(&_in_buffer, &_out_buffer);
        for (size_t i = 0; i < _input_ports.size(); ++i)
        {
            std::copy(_out_buffer.channel(i), _out_buffer.channel(i) + AUDIO_CHUNK_SIZE, out_data[i] + frames);
        }
    }
}

}; // end namespace audio_frontend
}; // end namespace sushi
#endif
#ifndef SUSHI_BUILD_WITH_JACK
#include <cassert>
#include "audio_frontends/jack_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
MIND_GET_LOGGER;
JackFrontend::JackFrontend(engine::BaseEngine* engine,
                           midi_dispatcher::MidiDispatcher* midi_dispatcher) : BaseAudioFrontend(engine, midi_dispatcher)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Jack support!");
    assert(false);
}}}
#endif
