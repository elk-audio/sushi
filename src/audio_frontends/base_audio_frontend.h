/**
 * @brief Base classes for audio frontends
 */
#ifndef SUSHI_BASE_AUDIO_FRONTEND_H
#define SUSHI_BASE_AUDIO_FRONTEND_H

#include "engine/engine.h"

namespace sushi {

namespace audio_frontend {

/**
 * @brief Error codes returned from init()
 */
enum class AudioFrontendInitStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_INPUT_FILE,
    INVALID_OUTPUT_FILE
};

/**
 * @brief Dummy base class to hold frontends configurations
 */
struct BaseAudioFrontendConfiguration
{
    BaseAudioFrontendConfiguration()
    {}

    virtual ~BaseAudioFrontendConfiguration()
    {}
};

/**
 * @brief Base class for Engine Frontends.
 */
class BaseAudioFrontend
{
public:
    BaseAudioFrontend(engine::EngineBase* engine) :
            _engine(engine)
    {}

    virtual ~BaseAudioFrontend()
    {}

    /**
     * @brief Initialize frontend with the given configuration.
     *        If anything can go wrong during initialization, partially allocated
     *        resources should be freed by calling cleanup().
     *
     * @param config Should be an object of the proper derived configuration class.
     * @return AudioFrontendInitStatus::OK in case of success,
     *         or different error code otherwise.
     */
    virtual AudioFrontendInitStatus init(BaseAudioFrontendConfiguration* config)
    {
        _config = config;
        return AudioFrontendInitStatus::OK;
    }

    /**
     * @brief Free resources allocated during init.
     */
    virtual void cleanup() = 0;

    /**
     * @brief Run engine main loop.
     */
    virtual void run() = 0;

protected:
    BaseAudioFrontendConfiguration* _config;
    engine::EngineBase* _engine;
};


}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_BASE_AUDIO_FRONTEND_H
