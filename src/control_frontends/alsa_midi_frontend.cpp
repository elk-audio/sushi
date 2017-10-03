#include "alsa_midi_frontend.h"
#include "logging.h"

MIND_GET_LOGGER;

namespace sushi {
namespace midi_frontend {

static const int ALSA_MAX_EVENT_SIZE_BYTES = 8192;
static const int ALSA_MIDI_POLL_TIMEOUT = 100;

AlsaMidiFrontend::AlsaMidiFrontend(midi_dispatcher::MidiDispatcher* dispatcher)
        : BaseMidiFrontend(dispatcher)
{

}

AlsaMidiFrontend::~AlsaMidiFrontend()
{
    /* Eventually when we switch to a polling version, we should join the midi thread here
     * Currently we can't since there is a blocking call to snd_seq_event_input */
    snd_midi_event_free(_seq_parser);
    snd_seq_close(_seq_handle);
}

bool AlsaMidiFrontend::init()
{
    auto alsamidi_ret = snd_seq_open(&_seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
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

    alsamidi_ret = snd_midi_event_new(ALSA_MAX_EVENT_SIZE_BYTES, &_seq_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating ALSA MIDI Event Parser: {}", strerror(-alsamidi_ret));
        return false;
    }
    _midi_buffer = std::unique_ptr<uint8_t>(new uint8_t[ALSA_MAX_EVENT_SIZE_BYTES]);
    return true;
}

void AlsaMidiFrontend::run()
{
    _running = true;
    _worker = std::thread(&AlsaMidiFrontend::_worker_function, this);

}

void AlsaMidiFrontend::stop()
{
    _running.store(false);
    /* Eventually when we switch to a polling version, we should join the midi thread here
     * Currently we can't since there is a blocking call to snd_seq_event_input */
}

void AlsaMidiFrontend::_worker_function()
{
    while (_running.load())
    {
        snd_seq_event_t *ev = nullptr;
        snd_seq_event_input(_seq_handle, &ev);

        if ( 	(ev->type == SND_SEQ_EVENT_NOTEON)
                || (ev->type == SND_SEQ_EVENT_NOTEOFF)
                || (ev->type == SND_SEQ_EVENT_CONTROLLER) )
        {
            const long num_bytes = snd_midi_event_decode (_seq_parser, _midi_buffer.get(),
                                                          ALSA_MAX_EVENT_SIZE_BYTES, ev);

            snd_midi_event_reset_decode(_seq_parser);
            _dispatcher->process_midi(0, 0, _midi_buffer.get(), num_bytes, false);
            snd_seq_free_event (ev);
        }
    }
}

} // end namespace midi_frontend
} // end namespace sushi

