/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

 /**
 * @brief Offline frontend to process audio files in chunks
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_OFFLINE_FRONTEND_H
#define SUSHI_OFFLINE_FRONTEND_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>

#include <sndfile.h>

#include "base_audio_frontend.h"
#include "library/rt_event.h"

namespace sushi {

namespace audio_frontend {

constexpr int OFFLINE_FRONTEND_CHANNELS = 2;
constexpr int DUMMY_FRONTEND_CHANNELS = 10;

struct OfflineFrontendConfiguration : public BaseAudioFrontendConfiguration
{

    OfflineFrontendConfiguration(const std::string input_filename,
                                 const std::string output_filename,
                                 bool dummy_mode,
                                 int cv_inputs,
                                 int cv_outputs) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            input_filename(input_filename),
            output_filename(output_filename),
            dummy_mode(dummy_mode)
    {}

    virtual ~OfflineFrontendConfiguration() = default;
    std::string input_filename;
    std::string output_filename;
    bool dummy_mode;
};

class OfflineFrontend : public BaseAudioFrontend
{
public:
    OfflineFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine),
                                                  _input_file(nullptr),
                                                  _output_file(nullptr),
                                                  _running{true}
    {
        _buffer.clear();
    }

    virtual ~OfflineFrontend()
    {
        cleanup();
    }

    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Add events that should be run during the processing
     * @param An std::vector containing the timestamped events.
     */
    void add_sequencer_events(std::vector<Event*> events);

    void cleanup() override;

    void run() override;

    void pause(bool enabled) override;

private:
    void _process_events(Time end_time);
    void _process_dummy();
    void _run_blocking();

    SNDFILE*            _input_file;
    SNDFILE*            _output_file;
    SF_INFO             _soundfile_info;
    bool                _mono;
    bool                _dummy_mode;
    std::atomic_bool    _running;
    std::thread         _worker;

    SampleBuffer<AUDIO_CHUNK_SIZE> _buffer{DUMMY_FRONTEND_CHANNELS};
    engine::ControlBuffer _control_buffer;

    std::vector<Event*> _event_queue;
};

} // end namespace audio_frontend

} // end namespace sushi

#endif //SUSHI_OFFLINE_FRONTEND_H
