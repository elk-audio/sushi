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
 * @brief Frontend using Xenomai rt framework
 *        and RTDM driver for audio over SPI
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_XENOMAI_RASPA_FRONTEND_H
#define SUSHI_XENOMAI_RASPA_FRONTEND_H

#ifdef SUSHI_BUILD_WITH_RASPA

#include "base_audio_frontend.h"

namespace sushi::internal::audio_frontend {

struct XenomaiRaspaFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiRaspaFrontendConfiguration(bool break_on_mode_sw,
                                      int cv_inputs,
                                      int cv_outputs) : BaseAudioFrontendConfiguration(cv_inputs, cv_outputs),
                                                        break_on_mode_sw(break_on_mode_sw) {}

    virtual ~XenomaiRaspaFrontendConfiguration() = default;
    bool break_on_mode_sw;
};

class XenomaiRaspaFrontend : public BaseAudioFrontend
{
public:
    explicit XenomaiRaspaFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~XenomaiRaspaFrontend()
    {
        cleanup();
    }

    /**
     * @brief Initialize the frontend and the audio driver.
     * @param config Configuration struct
     * @return OK on successful initialization, error otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Static callback passed to RASPA
     * @param input Input buffer in interleaved format
     * @param output Output buffer in interleaved format
     * @param data Opaque pointer to user data (this ptr in our case)
     */
    static void rt_process_callback(float* input, float* output, void* data)
    {
        return static_cast<XenomaiRaspaFrontend*>(data)->_internal_process_callback(input, output);
    }

    /**
     * @brief Call to clean up resources and release ports
     */
    void cleanup() override;

    /**
     * @brief Activate the realtime frontend, currently blocking.
     */
    void run() override;

    /**
     * @brief Workaround for Xenomai process initialization, which should happen
     *        as the _first_ thing in main() before everything else.
     *
     * @return 0 if successful, raspa_init() error code otherwise
     */
    static int global_init();

private:
    /* Internal process callback function */
    void _internal_process_callback(float* input, float* output);

    AudioFrontendStatus config_audio_channels(const XenomaiRaspaFrontendConfiguration* config);

    static bool _raspa_initialised;
    int _audio_input_channels;
    int _audio_output_channels;
    int _cv_input_channels;
    int _cv_output_channels;
    engine::ControlBuffer _in_controls;
    engine::ControlBuffer _out_controls;
    std::array<float, MAX_ENGINE_CV_IO_PORTS> _cv_output_hist{0};
};

} // end namespace sushi::internal::audio_frontend


#else // SUSHI_BUILD_WITH_RASPA
// Dummy frontend for non-Cobalt builds

#include "base_audio_frontend.h"

namespace sushi::internal::audio_frontend {

struct XenomaiRaspaFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiRaspaFrontendConfiguration(bool, int, int) : BaseAudioFrontendConfiguration(0, 0) {}
};

class XenomaiRaspaFrontend : public BaseAudioFrontend
{
public:
    XenomaiRaspaFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override;
    void cleanup() override {}
    void run() override {}
    void pause([[maybe_unused]] bool enabled) override {}
};

} // end namespace sushi::internal::audio_frontend

#endif // SUSHI_BUILD_WITH_RASPA
#endif // SUSHI_XENOMAI_RASPA_FRONTEND_H
