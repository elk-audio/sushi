#include <stdlib.h>
#include <alsa/seq_event.h>

#include "alsa_midi_frontend.h"
#include "logging.h"

MIND_GET_LOGGER;

namespace sushi {
namespace midi_frontend {

constexpr int ALSA_POLL_TIMEOUT_MS = 200;

AlsaMidiFrontend::AlsaMidiFrontend(midi_dispatcher::MidiDispatcher* dispatcher)
        : BaseMidiFrontend(dispatcher)
{}

AlsaMidiFrontend::~AlsaMidiFrontend()
{
    stop();
    snd_midi_event_free(_input_parser);
    snd_midi_event_free(_output_parser);
    snd_seq_close(_seq_handle);
}

bool AlsaMidiFrontend::init()
{
    auto alsamidi_ret = snd_seq_open(&_seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error opening Alsa MIDI port: {}", strerror(-alsamidi_ret));
        return false;
    }

    alsamidi_ret = snd_seq_set_client_name(_seq_handle, "Sushi");
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error setting ALSA client name: {}", strerror(-alsamidi_ret));
        return false;
    }
    _input_midi_port = snd_seq_create_simple_port(_seq_handle, "listen:in",
                                                  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                  SND_SEQ_PORT_TYPE_APPLICATION);
    if (_input_midi_port < 0)
    {
        MIND_LOG_ERROR("Error opening ALSA MIDI port: {}", strerror(-alsamidi_ret));
        return false;
    }

    _output_midi_port = snd_seq_create_simple_port(_seq_handle, "write:out",
                                                  SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                                                  SND_SEQ_PORT_TYPE_APPLICATION);
    if (_output_midi_port < 0)
    {
        MIND_LOG_ERROR("Error opening ALSA MIDI port: {}", strerror(-alsamidi_ret));
        return false;
    }

    alsamidi_ret = snd_midi_event_new(ALSA_MAX_EVENT_SIZE_BYTES, &_input_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating ALSA MIDI Input RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_midi_event_new(ALSA_MAX_EVENT_SIZE_BYTES, &_output_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating ALSA MIDI Output RtEvent Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    alsamidi_ret = snd_seq_nonblock(_seq_handle, 1);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Setting non-blocking mode failed: {}", strerror(-alsamidi_ret));
        return false;
    }

    snd_midi_event_no_status(_input_parser, 1); /* Disable running status in the decoder */
    snd_midi_event_no_status(_output_parser, 1); /* Disable running status in the encoder */
    return true;
}

void AlsaMidiFrontend::run()
{
    assert(_seq_handle);
    _running = true;
    _worker = std::thread(&AlsaMidiFrontend::_poll_function, this);
}

void AlsaMidiFrontend::stop()
{
    _running.store(false);
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void AlsaMidiFrontend::_poll_function()
{
    nfds_t descr_count = static_cast<nfds_t>(snd_seq_poll_descriptors_count(_seq_handle, POLLIN));
    auto descriptors = std::make_unique<pollfd[]>(descr_count);
    snd_seq_poll_descriptors(_seq_handle, descriptors.get(), static_cast<unsigned int>(descr_count), POLLIN);
    while (_running)
    {
        if (poll(descriptors.get(), descr_count, ALSA_POLL_TIMEOUT_MS) > 0)
        {
            snd_seq_event_t* ev{nullptr};
            uint8_t data_buffer[ALSA_MAX_EVENT_SIZE_BYTES]{0};
            while (snd_seq_event_input(_seq_handle, &ev) > 0)
            {
                if ((ev->type == SND_SEQ_EVENT_NOTEON)
                    || (ev->type == SND_SEQ_EVENT_NOTEOFF)
                    || (ev->type == SND_SEQ_EVENT_CONTROLLER))
                {
                    const long byte_count = snd_midi_event_decode(_input_parser, data_buffer,
                                                                  sizeof(data_buffer), ev);
                    if (byte_count > 0)
                    {
                        MIND_LOG_DEBUG("Alsa frontend: received midi message: [{:x} {:x} {:x} {:x}]", data_buffer[0], data_buffer[1], data_buffer[2], data_buffer[3]);
                        _dispatcher->process_midi(0, data_buffer, byte_count, 0);
                    }
                    MIND_LOG_WARNING_IF(byte_count < 0, "Alsa frontend: decoder returned {}", byte_count);
                }
                snd_seq_free_event(ev);
            }
        }
    }
}

void AlsaMidiFrontend::send_midi(int output, const uint8_t* data, int64_t timestamp)
{
    snd_seq_event ev;
    snd_seq_ev_clear(&ev);
    auto res = snd_midi_event_encode(_output_parser, data, 3, &ev);
    if (res <= 0 )
    {
        MIND_LOG_INFO("Alsa midi: failed to encode event: {} {}", strerror(-res), ev.type);
    }
    // TODO - Handle multiple output ports and add timestamps
    snd_seq_ev_set_source(&ev, _output_midi_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    res = snd_seq_event_output_direct(_seq_handle, &ev);
    if (res != 0)
    {
        MIND_LOG_INFO("Alsa midi: event output returned: {}, type {}", strerror(-res), ev.type);
    }
}

} // end namespace midi_frontend
} // end namespace sushi

