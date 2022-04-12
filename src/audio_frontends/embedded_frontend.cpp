/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Embedded frontend to process audio from a callback through a host application.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <iostream>

#include "logging.h"
#include "embedded_frontend.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("embedded audio frontend");

AudioFrontendStatus EmbeddedFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto embedded_config = static_cast<EmbeddedFrontendConfiguration*>(_config);

    _engine->set_audio_input_channels(EMBEDDED_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(EMBEDDED_FRONTEND_CHANNELS);

    auto status = _engine->set_cv_input_channels(embedded_config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv inputs failed", embedded_config->cv_inputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    status = _engine->set_cv_output_channels(embedded_config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv outputs failed", embedded_config->cv_outputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _engine->set_output_latency(std::chrono::microseconds(0));

    return ret_code;
}

void EmbeddedFrontend::cleanup()
{
    _engine->enable_realtime(false);
    _start_time_set = false; // TODO: More of a placeholder/reminder, to fix this when implementing "Pause".
}

void EmbeddedFrontend::run()
{
    _engine->enable_realtime(true);
}

// TODO: While in JUCE plugins channel count can change, in sushi it's set on init.
//  Also, in JUCE there seems to be support for having different numbers of input and output channels.
void EmbeddedFrontend::process_audio(ChunkSampleBuffer* in_buffer,
                                     ChunkSampleBuffer* out_buffer,
                                     int channel_count,
                                     int sample_count)
{
    if (!_start_time_set)
    {
        _start_time = twine::current_rt_time();
    }

    // TODO: Currently, while VST etc support dynamic buffer sizes, Sushi is hardcoded.
    //   These tests should really not be in process_audio, right?
    if (sample_count != AUDIO_CHUNK_SIZE)
    {
        assert(false);
        std::cout << "sample_count != AUDIO_CHUNK_SIZE, in embedded frontend." << std::endl;
        return;
    }

    if (channel_count != EMBEDDED_FRONTEND_CHANNELS)
    {
        assert(false);
        std::cout << "Channel count passed is different to EMBEDDED_FRONTEND_CHANNELS, in embedded frontend." << std::endl;
        return;
    }

    // TODO: Deal also with MIDI.

    // TODO: Deal also with CV.

    // TODO: This doesn't seem to work as expected - The LV2 metro plugin isn't working with this frontend.
    auto timestamp = std::chrono::duration_cast<Time>(twine::current_rt_time() - _start_time);

    out_buffer->clear();

    _engine->process_chunk(in_buffer,
                           out_buffer,
                           &_in_controls,
                           &_out_controls,
                           timestamp,
                           sample_count);
}

} // end namespace audio_frontend

} // end namespace sushi