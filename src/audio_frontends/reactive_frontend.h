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
 * @brief Reactive frontend to process audio from a callback through a host application.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_REACTIVE_FRONTEND_H
#define SUSHI_REACTIVE_FRONTEND_H

#include <string>
#include <vector>
#include <atomic>
#include <thread>

#include <sndfile.h>

#include "base_audio_frontend.h"
#include "library/rt_event.h"

namespace sushi::internal::audio_frontend {

/**
 * @brief Maximum number of audio channels (each way) supported by the
 *        Reactive frontend. Kept separate from MAX_FRONTEND_CHANNELS so the
 *        higher reactive limit does not change the channel/port count of the
 *        other frontends (e.g. the JACK frontend registers
 *        MAX_FRONTEND_CHANNELS ports).
 */
constexpr int MAX_REACTIVE_CHANNELS = 16;

struct ReactiveFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    ReactiveFrontendConfiguration(int audio_inputs,
                                  int audio_outputs,
                                  int cv_inputs,
                                  int cv_outputs,
                                  int output_latency_us = 0) :
            BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
            audio_inputs{audio_inputs},
            audio_outputs{audio_outputs},
            output_latency_us{output_latency_us}
    {}

    ~ReactiveFrontendConfiguration() override = default;

    int audio_inputs;
    int audio_outputs;
    int output_latency_us;
};

class ReactiveFrontend : public BaseAudioFrontend
{
public:
    explicit ReactiveFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    ~ReactiveFrontend() override
    {
        cleanup();
    }

    /**
     * @brief Initialize frontend with the given configuration.
     *        If anything can go wrong during initialization, partially allocated
     *        resources should be freed by calling cleanup().
     *
     * @param config Should be an object of the proper derived configuration class.
     * @return AudioFrontendInitStatus::OK in case of success,
     *         or different error code otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Free resources allocated during init. stops the frontend if currently running.
     */
    void cleanup() override;

    /**
     * @brief Run engine main loop.
     */
    void run() override;

    /**
     * @brief Method to invoke from the host's audio callback.
     * @param in_buffer Input sample buffer
     * @param out_buffer Output sample buffer
     * @param channel_count number of audio channels
     * @param total_sample_count since start (timestamp)
     * @param timestamp timestamp for call
     */
     void process_audio(ChunkSampleBuffer& in_buffer,
                        ChunkSampleBuffer& out_buffer,
                        int64_t total_sample_count,
                        Time timestamp);

     /**
     * @brief Call before the first call to process_audio() when resuming from an interrupt or xrun to
     *        notify sushi that audio processing was interrupted and that there may be gaps in the audio
     * @param duration The length of the interruption
     */
     void notify_interrupted_audio(Time duration);

    /**
     * @brief Set a CV input value to be passed to the engine in the next process_audio() call.
     *        Call this before process_audio() from the audio thread.
     * @param channel Index in [0, MAX_ENGINE_CV_IO_PORTS).
     * @param value   Normalised value [0.0, 1.0].
     */
    void set_cv_input(int channel, float value)
    {
        _in_controls.cv_values[static_cast<size_t>(channel)] = value;
    }

    /**
     * @brief Read a CV output value produced by the engine during the last process_audio() call.
     * @param channel Index in [0, MAX_ENGINE_CV_IO_PORTS).
     * @return Normalised value [0.0, 1.0].
     */
    [[nodiscard]] float cv_output(int channel) const
    {
        return _out_controls.cv_values[static_cast<size_t>(channel)];
    }

    /**
     * @brief Set a gate (digital) input line state before the next process_audio() call.
     * @param gate  Index in [0, 32).
     * @param high  true = high, false = low.
     */
    void set_gate_input(int gate, bool high)
    {
        _in_controls.gate_values.set(static_cast<size_t>(gate), high);
    }

    /**
     * @brief Read a gate output state produced by the engine during the last process_audio() call.
     * @param gate Index in [0, 32).
     * @return true if the gate is high.
     */
    [[nodiscard]] bool gate_output(int gate) const
    {
        return _out_controls.gate_values.test(static_cast<size_t>(gate));
    }

private:
    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;
};

} // end namespace sushi::internal::audio_frontend

#endif // SUSHI_REACTIVE_FRONTEND_H
