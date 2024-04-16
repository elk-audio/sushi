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

    _engine->set_audio_input_channels(REACTIVE_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(REACTIVE_FRONTEND_CHANNELS);

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

    _engine->set_output_latency(std::chrono::microseconds(0));

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

// TODO: While in JUCE plugins channel count can change, in sushi it's set on init.
//  In JUCE, the buffer size is always the same for in and out, with some unused,
//  if they differ.
void ReactiveFrontend::process_audio(ChunkSampleBuffer& in_buffer,
                                     ChunkSampleBuffer& out_buffer,
                                     int64_t total_sample_count,
                                     Time timestamp)
{
    // TODO: Do we need to concern ourselves with multiple buses?

    // TODO: Deal also with MIDI.

    // TODO: Deal also with CV.

    out_buffer.clear();

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
}

void ReactiveFrontend::notify_interrupted_audio(Time duration)
{
    _engine->notify_interrupted_audio(duration);
}

} // end namespace sushi::internal::audio_frontend