/*
 * Copyright 2017-2023 Elk Audio AB
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
* @brief Realtime audio frontend for Apple CoreAudio
* @Copyright 2017-2023 Elk Audio AB, Stockholm
*/

#ifndef SUSHI_APPLE_COREAUDIO_FRONTEND_H
#define SUSHI_APPLE_COREAUDIO_FRONTEND_H

#ifdef SUSHI_BUILD_WITH_APPLE_COREAUDIO

#include <memory>

#include "base_audio_frontend.h"
#include "apple_coreaudio/apple_coreaudio_object.h"
#include "apple_coreaudio/apple_coreaudio_device.h"

// See AppleCoreAudioFrontend::sample_rate_changed()
#define EXIT_SUSHI_WHEN_AUDIO_DEVICE_CHANGES_TO_INCOMPATIBLE_SAMPLE_RATE

namespace sushi::internal::audio_frontend {

[[nodiscard]] std::optional<std::string> get_coreaudio_output_device_name(std::optional<std::string> coreaudio_output_device_uid);

struct AppleCoreAudioFrontendConfiguration : public BaseAudioFrontendConfiguration {
    AppleCoreAudioFrontendConfiguration(std::optional<std::string> input_device_uid,
                                        std::optional<std::string> output_device_uid,
                                        int cv_inputs,
                                        int cv_outputs) : BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
                                                          input_device_uid(std::move(input_device_uid)),
                                                          output_device_uid(std::move(output_device_uid))
    {}

    ~AppleCoreAudioFrontendConfiguration() override = default;

    std::optional<std::string> input_device_uid;
    std::optional<std::string> output_device_uid;
};

class AppleCoreAudioFrontend : public BaseAudioFrontend, private apple_coreaudio::AudioDevice::AudioCallback
{
public:
    explicit AppleCoreAudioFrontend(engine::BaseEngine* engine);
    ~AppleCoreAudioFrontend() override = default;

    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;
    void cleanup() override;
    void run() override;

    AudioFrontendStatus configure_audio_channels(const AppleCoreAudioFrontendConfiguration* config);

    bool start_io();
    bool stop_io();

private:
    std::unique_ptr<apple_coreaudio::AudioDevice> _audio_device;
    apple_coreaudio::TimeConversions _time_conversions;
    int _device_num_input_channels{0};
    int _device_num_output_channels{0};
    ChunkSampleBuffer _in_buffer{0};
    ChunkSampleBuffer _out_buffer{0};
    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;
    int64_t _processed_sample_count{0};

    // See the body of sample_rate_changed for an explanation for this variable.
    // Note: Elk LIVE Desktop depends on this specific return value.
    static constexpr int EXIT_RETURN_VALUE_ON_INCOMPATIBLE_SAMPLE_RATE_CHANGE = 55;

    void _copy_interleaved_audio_to_input_buffer(const float* input, int num_channels);
    void _copy_output_buffer_to_interleaved_buffer(float* output, int num_channels);

    // apple_coreaudio::AudioDevice::AudioCallback overrides
    void audio_callback(const float* input_data, int num_input_channels, float* output_data, int num_output_channels, int num_samples, uint64_t host_input_time) override;

    void sample_rate_changed(double new_sample_rate) override;
};

} // namespace sushi::internal::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO

#ifndef SUSHI_BUILD_WITH_APPLE_COREAUDIO
/* If Apple CoreAudio is disabled in the build config, the AppleCoreAudio frontend is replaced with
   this dummy frontend whose only purpose is to assert if you try to use it */

#include "base_audio_frontend.h"

namespace sushi::internal::audio_frontend {

struct AppleCoreAudioFrontendConfiguration : public BaseAudioFrontendConfiguration {
    AppleCoreAudioFrontendConfiguration(const std::optional<std::string>&,
                                        const std::optional<std::string>&,
                                        int,
                                        int) : BaseAudioFrontendConfiguration(0, 0) {}
};

class AppleCoreAudioFrontend : public BaseAudioFrontend
{
public:
    explicit AppleCoreAudioFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {};
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    void cleanup() override{};
    void run() override{};
};

} // namespace sushi::internal::audio_frontend

#endif // SUSHI_BUILD_WITH_APPLE_COREAUDIO

#endif // SUSHI_APPLE_COREAUDIO_FRONTEND_H
