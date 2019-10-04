/**
 * @brief Frontend using Xenomai with RASPA library for XMOS board.
 *
 */
#ifdef SUSHI_BUILD_WITH_XENOMAI

#include <cerrno>

#include "raspa/raspa.h"

#include "xenomai_raspa_frontend.h"
#include "audio_frontend_internals.h"
#include "logging.h"

namespace sushi {
namespace audio_frontend {

MIND_GET_LOGGER_WITH_MODULE_NAME("raspa audio");

bool XenomaiRaspaFrontend::_raspa_initialised = false;

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }
    auto raspa_config = static_cast<XenomaiRaspaFrontendConfiguration*>(_config);

    // RASPA
    if (RASPA_N_FRAMES_PER_BUFFER != AUDIO_CHUNK_SIZE)
    {
        MIND_LOG_ERROR("Chunk size mismatch, check driver configuration.");
        return AudioFrontendStatus::INVALID_CHUNK_SIZE;
    }
    _engine->set_audio_input_channels(RASPA_N_CHANNELS);
    _engine->set_audio_output_channels(RASPA_N_CHANNELS);
    _engine->set_output_latency(std::chrono::microseconds(raspa_get_output_latency()));
    if (_engine->sample_rate() != RASPA_AUDIO_SAMPLE_RATE)
    {
        MIND_LOG_WARNING("Sample rate mismatch between engine ({}) and Raspa ({})", _engine->sample_rate(), RASPA_AUDIO_SAMPLE_RATE);
        _engine->set_sample_rate(RASPA_AUDIO_SAMPLE_RATE);
    }
    unsigned int debug_flags = 0;
    if (raspa_config->break_on_mode_sw)
    {
        debug_flags |= RASPA_DEBUG_SIGNAL_ON_MODE_SW;
    }

    auto raspa_ret = raspa_open(RASPA_N_CHANNELS, RASPA_N_FRAMES_PER_BUFFER, rt_process_callback, this, debug_flags);
    if (raspa_ret < 0)
    {
        MIND_LOG_ERROR("Error opening RASPA: {}", strerror(-raspa_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    return AudioFrontendStatus::OK;
}

void XenomaiRaspaFrontend::cleanup()
{
    if (_raspa_initialised)
    {
        MIND_LOG_INFO("Closing Raspa driver.");
        raspa_close();
    }
    _raspa_initialised = false;
}

void XenomaiRaspaFrontend::run()
{
    raspa_start_realtime();
}

int XenomaiRaspaFrontend::global_init()
{
    auto status = raspa_init();
    _raspa_initialised = status == 0;
    return status;
}

void XenomaiRaspaFrontend::_internal_process_callback(float* input, float* output)
{
    Time timestamp = Time(raspa_get_time());
    set_flush_denormals_to_zero();
    int64_t samplecount = raspa_get_samplecount();
    _engine->update_time(timestamp, samplecount);

    ChunkSampleBuffer in_buffer = ChunkSampleBuffer::create_from_raw_pointer(input, 0, RASPA_N_CHANNELS);
    ChunkSampleBuffer out_buffer = ChunkSampleBuffer::create_from_raw_pointer(output, 0, RASPA_N_CHANNELS);
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
XenomaiRaspaFrontend::XenomaiRaspaFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Xenomai Cobalt support!");
    assert(false);
}}}

#endif // SUSHI_BUILD_WITH_XENOMAI
