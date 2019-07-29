/**
 * @brief Frontend using Xenomai rt framework
 *        and RTDM driver for audio over SPI
 */

#ifndef SUSHI_XENOMAI_RASPA_FRONTEND_H
#define SUSHI_XENOMAI_RASPA_FRONTEND_H

#ifdef SUSHI_BUILD_WITH_XENOMAI

#include "base_audio_frontend.h"

namespace sushi {
namespace audio_frontend {

struct XenomaiRaspaFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiRaspaFrontendConfiguration(bool break_on_mode_sw) : break_on_mode_sw(break_on_mode_sw) {}

    virtual ~XenomaiRaspaFrontendConfiguration() = default;
    bool break_on_mode_sw;
};

class XenomaiRaspaFrontend : public BaseAudioFrontend
{
public:
    XenomaiRaspaFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

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

    static bool _raspa_initialised;
};

}; // end namespace audio_frontend
}; // end namespace sushi


#else // SUSHI_BUILD_WITH_XENOMAI
// Dummy frontend for non-Cobalt builds

#include "base_audio_frontend.h"
namespace sushi {
namespace audio_frontend {
struct XenomaiRaspaFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiRaspaFrontendConfiguration(bool) {}
};

class XenomaiRaspaFrontend : public BaseAudioFrontend
{
public:
    XenomaiRaspaFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override {return AudioFrontendStatus::OK;}
    void cleanup() override {}
    void run() override {}
};

}; // end namespace audio_frontend
}; // end namespace sushi

#endif // SUSHI_BUILD_WITH_XENOMAI
#endif // SUSHI_XENOMAI_RASPA_FRONTEND_H
