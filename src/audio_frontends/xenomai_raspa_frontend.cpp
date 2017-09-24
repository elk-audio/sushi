/**
 * @brief Frontend using Xenomai with RASPA library for XMOS board.
 *
 */
#ifdef SUSHI_BUILD_WITH_XENOMAI

#include <unistd.h>
#include <errno.h>

#include "xenomai_raspa_frontend.h"
#include "logging.h"

#include "raspa.h"

namespace sushi {
namespace audio_frontend {

static const int ALSA_MAX_EVENT_SIZE_BYTES = 8192;
static const int ALSA_MIDI_POLL_TIMEOUT = 100;

MIND_GET_LOGGER;

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    // RASPA
    if (RASPA_N_FRAMES_PER_BUFFER != AUDIO_CHUNK_SIZE)
    {
        MIND_LOG_ERROR("Chunk size mismatch, check driver configuration.");
        return AudioFrontendStatus::INVALID_CHUNK_SIZE;
    }
    if (RASPA_N_CHANNELS != MAX_FRONTEND_CHANNELS)
    {
        MIND_LOG_ERROR("Number of channels mismatch, check driver configuration.");
        return AudioFrontendStatus::INVALID_N_CHANNELS;
    }

    auto init_ret = raspa_init();
    if (init_ret < 0)
    {
        MIND_LOG_ERROR("Error initializing RASPA: {}", strerror(-init_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto raspa_ret = raspa_open(RASPA_N_CHANNELS, RASPA_N_FRAMES_PER_BUFFER, rt_process_callback, this);
    if (raspa_ret < 0)
    {
        MIND_LOG_ERROR("Error opening RASPA: {}", strerror(-raspa_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    // Control
    _osc_control = std::make_unique<control_frontend::OSCFrontend>(&_event_queue, _engine);

    auto alsamidi_ret = snd_seq_open(&_seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error opening Alsa MIDI port: {}", strerror(-alsamidi_ret));
        return AudioFrontendStatus::MIDI_PORT_ERROR;
    }

    alsamidi_ret = snd_seq_set_client_name(_seq_handle, "Sushi");
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error setting ALSA client name: {}", strerror(-alsamidi_ret));
        return AudioFrontendStatus::MIDI_PORT_ERROR;
    }
    _input_midi_port = snd_seq_create_simple_port(_seq_handle, "listen:in",
                                                  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                  SND_SEQ_PORT_TYPE_APPLICATION);
    if (_input_midi_port < 0)
    {
        MIND_LOG_ERROR("Error opening ALSA MIDI port: {}", strerror(-alsamidi_ret));
        return AudioFrontendStatus::MIDI_PORT_ERROR;
    }

    alsamidi_ret = snd_midi_event_new(ALSA_MAX_EVENT_SIZE_BYTES, &_seq_parser);
    if (alsamidi_ret < 0)
    {
        MIND_LOG_ERROR("Error creating ALSA MIDI Event Parser: {}", strerror(-alsamidi_ret));
        return AudioFrontendStatus::MIDI_PORT_ERROR;
    }
    _midi_buffer = std::unique_ptr<uint8_t>(new uint8_t[ALSA_MAX_EVENT_SIZE_BYTES]);

    return AudioFrontendStatus::OK;
}


void XenomaiRaspaFrontend::cleanup()
{
    _osc_control->stop();
    snd_midi_event_free(_seq_parser);
    snd_seq_close(_seq_handle);

    MIND_LOG_INFO("Closing Raspa driver.");
    raspa_close();
}


void XenomaiRaspaFrontend::run()
{
    _osc_control->run();
    _osc_control->connect_all();

    // TODO temp solution until we can work out the poll-based version
    while (true)
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
            _midi_dispatcher->process_midi(0, 0, _midi_buffer.get(), num_bytes, false);
            snd_seq_free_event (ev);
        }
    }

}


void XenomaiRaspaFrontend::_internal_process_callback(float* input, float* output)
{
    while (!_event_queue.empty())
    {
        Event event;
        if (_event_queue.pop(event))
        {
            _engine->send_rt_event(event);
        }
    }

    ChunkSampleBuffer in_buffer = ChunkSampleBuffer::create_from_raw_pointer(input, 0, 2);
    ChunkSampleBuffer out_buffer = ChunkSampleBuffer::create_from_raw_pointer(output, 0, 2);
    out_buffer.clear();
    _engine->process_chunk(&in_buffer, &out_buffer);
}


}; // end namespace audio_frontend
}; // end namespace sushi

#else // SUSHI_BUILD_WITH_XENOMAI

#include <cassert>
#include "audio_frontends/xenomai_raspa_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
MIND_GET_LOGGER;
XenomaiRaspaFrontend::XenomaiRaspaFrontend(engine::BaseEngine* engine,
                                midi_dispatcher::MidiDispatcher* midi_dispatcher) : BaseAudioFrontend(engine, midi_dispatcher)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Xenomai Cobalt support!");
    assert(false);
}}}

#endif // SUSHI_BUILD_WITH_XENOMAI
