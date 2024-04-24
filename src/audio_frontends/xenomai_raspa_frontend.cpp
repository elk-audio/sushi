/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

 /**
  * @brief Frontend using Xenomai with RASPA library for XMOS board.
  * @Copyright 2017-2023 Elk Audio AB, Stockholm
  */

#ifdef SUSHI_BUILD_WITH_RASPA

#include <cerrno>

#include <raspa/raspa.h>

#include "xenomai_raspa_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi::internal::audio_frontend {

/**
 * Ensure version compatibility with raspa library
 */
constexpr int REQUIRED_RASPA_VER_MAJ = 1;
constexpr int REQUIRED_RASPA_VER_MIN = 1;
static_assert(REQUIRED_RASPA_VER_MAJ == RASPA_VERSION_MAJ, "Raspa major version mismatch");
static_assert(REQUIRED_RASPA_VER_MIN == RASPA_VERSION_MIN, "Raspa minor version mismatch");

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("raspa audio");

bool XenomaiRaspaFrontend::_raspa_initialised = false;

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto raspa_config = static_cast<const XenomaiRaspaFrontendConfiguration*>(_config);

    unsigned int debug_flags = 0;
    if (raspa_config->break_on_mode_sw)
    {
        debug_flags |= RASPA_DEBUG_SIGNAL_ON_MODE_SW;
    }

    auto raspa_ret = raspa_open(AUDIO_CHUNK_SIZE, rt_process_callback, this, debug_flags);
    if (raspa_ret < 0)
    {
        ELKLOG_LOG_ERROR("Error opening RASPA: {}", raspa_get_error_msg(-raspa_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto cv_audio_status = config_audio_channels(raspa_config);
    if (cv_audio_status != AudioFrontendStatus::OK)
    {
        ELKLOG_LOG_ERROR("Incompatible cv and audio channel setup");
        return cv_audio_status;
    }

    auto raspa_sample_rate = raspa_get_sampling_rate();
    if (raspa_sample_rate == 0)
    {
        ELKLOG_LOG_ERROR("Raspa has invalid sample rate of 0.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    ELKLOG_LOG_WARNING_IF(_engine->sample_rate() != raspa_sample_rate,
                          "Sample rate mismatch between engine ({}) and Raspa ({}), setting to {}",
                          _engine->sample_rate(), raspa_sample_rate, raspa_sample_rate);

    _set_engine_sample_rate(raspa_sample_rate);
    _engine->set_output_latency(std::chrono::microseconds(raspa_get_output_latency()));

    return AudioFrontendStatus::OK;
}

void XenomaiRaspaFrontend::cleanup()
{
    _engine->enable_realtime(false);
    if (_raspa_initialised)
    {
        ELKLOG_LOG_INFO("Closing Raspa driver.");
        raspa_close();
    }
    _raspa_initialised = false;
}

void XenomaiRaspaFrontend::run()
{
    _engine->enable_realtime(true);
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

    // Gate in signals from the Sika board are inverted, hence invert all bits
    _in_controls.gate_values = ~engine::BitSet32(raspa_get_gate_values());

    ChunkSampleBuffer in_buffer = ChunkSampleBuffer::create_from_raw_pointer(input, 0, _audio_input_channels);
    ChunkSampleBuffer out_buffer = ChunkSampleBuffer::create_from_raw_pointer(output, 0, _audio_output_channels);
    for (int i = 0; i < _cv_input_channels; ++i)
    {
        _in_controls.cv_values[i] = map_audio_to_cv(input[(_audio_input_channels + i + 1) * AUDIO_CHUNK_SIZE - 1] * CV_IN_CORR);
    }

    out_buffer.clear();
    _handle_resume(timestamp, AUDIO_CHUNK_SIZE);

    if (_pause_manager.should_process())
    {
        _engine->process_chunk(&in_buffer, &out_buffer, &_in_controls, &_out_controls, timestamp, samplecount);
        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(out_buffer);
        }
       raspa_set_gate_values(static_cast<uint32_t>(_out_controls.gate_values.to_ulong()));
        /* Sika board outputs only positive cv */
        for (int i = 0; i < _cv_output_channels; ++i)
        {
            float* out_data = output + (_audio_output_channels + i) * AUDIO_CHUNK_SIZE;
            _cv_output_hist[i] = ramp_cv_output(out_data, _cv_output_hist[i], _out_controls.cv_values[i] * CV_OUT_CORR);
        }
    }

    _handle_pause(timestamp);
}

AudioFrontendStatus XenomaiRaspaFrontend::config_audio_channels(const XenomaiRaspaFrontendConfiguration* config)
{
    /* CV channels ar counted from the back, so if RASPA_N_CHANNELS is 8 and
     * cv inputs is set to 2, The engine will be set to 6 audio input channels
     * and the last 2 will be used as cv input 0 and cv input 1, respectively
     * In the first revision Sika, CV outs are on channels 4 and 5 (counted from 0) and
     * optional on 6 and 7, so only 0 or 4 cv channels is accepted */
    if (config->cv_inputs != 0 && config->cv_inputs != 2)
    {
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    if (config->cv_outputs != 0 && config->cv_outputs != 4)
    {
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto num_total_input_channels = raspa_get_num_input_channels();
    auto num_total_output_channels = raspa_get_num_output_channels();
    if (num_total_input_channels == 0 && num_total_output_channels == 0)
    {
        ELKLOG_LOG_ERROR("RASPA has no input or output channels.");
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _cv_input_channels = config->cv_inputs;
    _cv_output_channels = config->cv_outputs;
    _audio_input_channels = num_total_input_channels - _cv_input_channels;
    _audio_output_channels = num_total_output_channels - _cv_output_channels;
    _engine->set_audio_input_channels(_audio_input_channels);
    _engine->set_audio_output_channels(_audio_output_channels);
    auto status = _engine->set_cv_input_channels(_cv_input_channels);
    if (status != engine::EngineReturnStatus::OK)
    {
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    status = _engine->set_cv_output_channels(_cv_output_channels);
    if (status != engine::EngineReturnStatus::OK)
    {
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }
    return AudioFrontendStatus::OK;
}

} // end namespace sushi::internal::audio_frontend

#else // SUSHI_BUILD_WITH_RASPA

#include "elklog/static_logger.h"

#include "xenomai_raspa_frontend.h"

namespace sushi::internal::audio_frontend {

ELKLOG_GET_LOGGER;

XenomaiRaspaFrontend::XenomaiRaspaFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine)
{}

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration*)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    ELKLOG_LOG_ERROR("Sushi was not built with Xenomai Cobalt support!");
    return AudioFrontendStatus::AUDIO_HW_ERROR;
}

}

#endif // SUSHI_BUILD_WITH_RASPA
