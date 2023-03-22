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
 * @brief Passive frontend to process audio from a callback through a host application.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <iostream>

#include "logging.h"
#include "passive_frontend.h"

namespace sushi {
namespace audio_frontend {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("passive audio frontend");

AudioFrontendStatus PassiveFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    auto frontend_config = static_cast<PassiveFrontendConfiguration*>(_config);

    _in_buffer = ChunkSampleBuffer(PASSIVE_FRONTEND_CHANNELS);
    _out_buffer = ChunkSampleBuffer(PASSIVE_FRONTEND_CHANNELS);

    _engine->set_audio_input_channels(PASSIVE_FRONTEND_CHANNELS);
    _engine->set_audio_output_channels(PASSIVE_FRONTEND_CHANNELS);

    auto status = _engine->set_cv_input_channels(frontend_config->cv_inputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv inputs failed", frontend_config->cv_inputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    status = _engine->set_cv_output_channels(frontend_config->cv_outputs);
    if (status != engine::EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Setting {} cv outputs failed", frontend_config->cv_outputs);
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    _engine->set_output_latency(std::chrono::microseconds(0));

    return ret_code;
}

void PassiveFrontend::cleanup()
{
    _engine->enable_realtime(false);
}

void PassiveFrontend::run()
{
    _engine->enable_realtime(true);
}

// TODO: While in JUCE plugins channel count can change, in sushi it's set on init.
//  In JUCE, the buffer size is always the same for in and out, with some unused,
//  if they differ.
void PassiveFrontend::process_audio(int channel_count,
                                    int total_sample_count,
                                    Time timestamp)
{
    // TODO: Do we need to concern ourselves with multiple buses?

    if (channel_count != PASSIVE_FRONTEND_CHANNELS)
    {
        assert(false);
        std::cout << "Channel count passed is different to PASSIVE_FRONTEND_CHANNELS, in passive frontend." << std::endl;
        return;
    }

    // TODO: Deal also with MIDI.

    // TODO: Deal also with CV.

    _out_buffer.clear();

    if (_pause_manager.should_process())
    {
        _engine->process_chunk(&_in_buffer,
                               &_out_buffer,
                               &_in_controls,
                               &_out_controls,
                               timestamp,
                               total_sample_count);

        if (_pause_manager.should_ramp())
        {
            _pause_manager.ramp_output(_out_buffer);
        }
    }
    else
    {
        if (_pause_notified == false && _pause_manager.should_process() == false)
        {
            _pause_notify->notify();
            _pause_notified = true;
        }
    }
}

ChunkSampleBuffer& PassiveFrontend::in_buffer()
{
    return _in_buffer;
}

ChunkSampleBuffer& PassiveFrontend::out_buffer()
{
    return _out_buffer;
}

} // end namespace audio_frontend

} // end namespace sushi