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

MIND_GET_LOGGER;

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }
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

   _osc_control = std::make_unique<control_frontend::OSCFrontend>(&_event_queue, _engine);

    return AudioFrontendStatus::OK;
}


void XenomaiRaspaFrontend::cleanup()
{
    raspa_close();
}


void XenomaiRaspaFrontend::run()
{
// TODO:
// After UART protocol for starting/stopping XMOS is implemented, we can trigger
// audio stream start from here
    _osc_control->run();
// TODO: change this (both here and in Jack frontend) with something that can be interrupted,
//       e.g. checking a flag that can be set with signals or OSC
    sleep(10000);
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

    _in_buffer.from_interleaved(input);
    _out_buffer.clear();
    _engine->process_chunk(&_in_buffer, &_out_buffer);
    _out_buffer.to_interleaved(output);
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
