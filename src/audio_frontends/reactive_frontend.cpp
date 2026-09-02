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
 * @brief Passive frontend to process audio from a callback through a host application.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <iostream>

#include "elklog/static_logger.h"

#include "reactive_frontend.h"
#include "audio_frontend_internals.h"

namespace sushi::internal::audio_frontend {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("Reactive audio frontend");

AudioFrontendStatus ReactiveFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto frontend_config = static_cast<ReactiveFrontendConfiguration*>(_config); // static cast because of no rtti

    if (frontend_config->audio_inputs > MAX_REACTIVE_CHANNELS ||
        frontend_config->audio_outputs > MAX_REACTIVE_CHANNELS)
    {
        ELKLOG_LOG_ERROR("Requested channel count ({} in / {} out) exceeds MAX_REACTIVE_CHANNELS ({})",
                         frontend_config->audio_inputs,
                         frontend_config->audio_outputs,
                         MAX_REACTIVE_CHANNELS);
        return AudioFrontendStatus::INVALID_N_CHANNELS;
    }

    _engine->set_audio_channels(frontend_config->audio_inputs, frontend_config->audio_outputs);

    auto status = _engine->set_cv_input_channels(frontend_config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        ELKLOG_LOG_ERROR("Setting {} cv inputs failed", frontend_config->cv_inputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    status = _engine->set_cv_output_channels(frontend_config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        ELKLOG_LOG_ERROR("Setting {} cv outputs failed", frontend_config->cv_outputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _engine->set_output_latency(std::chrono::microseconds(frontend_config->output_latency_us));

    return ret_code;
}

void ReactiveFrontend::cleanup()
{
    _engine->enable_realtime(false);
}

void ReactiveFrontend::run()
{
    _engine->enable_realtime(true);
}

// Note: channel count changes at runtime are not supported — the count is fixed at init().
// Buffer sizes other than AUDIO_CHUNK_SIZE are the host's responsibility to handle.
void ReactiveFrontend::process_audio(ChunkSampleBuffer& in_buffer,
                                     ChunkSampleBuffer& out_buffer,
                                     int64_t total_sample_count,
                                     Time timestamp)
{
    // Keep the FTZ/DAZ flags set on x86; no-op on ARM where the compiler/ABI handles it.
    set_flush_denormals_to_zero();

    out_buffer.clear();

    // Automatic xrun detection: compares timestamp against the previous callback.
    // Fires engine->notify_interrupted_audio() when a gap is detected.
    // This only has sub-block precision when the host passes real hardware timestamps
    // (e.g. twine::current_rt_time()) — with calculated timestamps xruns are invisible.
    // Also handles the resume-after-pause notification path.
    _handle_resume(timestamp, AUDIO_CHUNK_SIZE);

    if (_pause_manager.should_process())
    {
        _engine->process_chunk(&in_buffer,
                               &out_buffer,
                               &_in_controls,
                               &_out_controls,
                               timestamp,
                               total_sample_count);

        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(out_buffer);
        }
    }

    // Notify the non-RT thread blocked in BaseAudioFrontend::pause(true) once the
    // output has ramped down.  Without this call, pause() would deadlock.
    _handle_pause(timestamp);
}

void ReactiveFrontend::notify_interrupted_audio(Time duration)
{
    _engine->notify_interrupted_audio(duration);
}

} // end namespace sushi::internal::audio_frontend