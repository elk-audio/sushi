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

#include <cmath>
#include <cstring>
#include <random>

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
    // TODO: FILL
}

void EmbeddedFrontend::run()
{
    // TODO: FILL
}

void EmbeddedFrontend::_process_events(Time end_time)
{
    // TODO: FILL
}

} // end namespace audio_frontend

} // end namespace sushi