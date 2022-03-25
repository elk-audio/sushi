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

    // TODO: FILL
}

void EmbeddedFrontend::run()
{
    _engine->enable_realtime(true);
    _start_time = twine::current_rt_time();

    // TODO: FILL
}

void EmbeddedFrontend::process_audio(const float** array_of_read_pointers,
                                     float** array_of_write_pointers,
                                     int channel_count, // TODO: channel count should be static, set in init, no?
                                     int sample_count)
{
    // TODO: Currently, while VST etc support dynamic buffer sizes, Sushi is hardcoded!
    if (sample_count != AUDIO_CHUNK_SIZE)
    {
        assert(sample_count == AUDIO_CHUNK_SIZE);
        std::cout << "sample_count != AUDIO_CHUNK_SIZE, in embedded frontend." << std::endl;
        return;
    }

    _in_buffer.clear();
    for (int c = 0; c < channel_count; ++c)
    {
        const float* in_src = array_of_read_pointers[c];

        float* in_dst = _in_buffer.channel(c); // In Ruben's code this was inside the for loop, can't see why.

        for (int s = 0; s < sample_count; ++s)
        {
            in_dst[s] = in_src[s];
        }
    }

    // TODO: Deal with CV!

    // TODO: Do I need to COPY the buffers, can't I just use the raw pointers!?

    auto timestamp = std::chrono::duration_cast<Time>(twine::current_rt_time() - _start_time);

    _out_buffer.clear();
    _engine->process_chunk(&_in_buffer,
                           &_out_buffer,
                           &_in_controls,
                           &_out_controls,
                           timestamp,
                           sample_count);

    for (int c = 0; c < channel_count; ++c)
    {
        float* out_dst = array_of_write_pointers[c];

        const float* out_src = _out_buffer.channel(c); // TODO: in Ruben's code this was inside the for loop, can't see why.

        for (int s = 0; s < sample_count; ++s)
        {
            out_dst[s] = out_src[s];
        }
    }
}

} // end namespace audio_frontend

} // end namespace sushi