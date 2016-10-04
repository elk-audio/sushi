/**
 * @brief Base classes for audio frontends
 */
#ifndef SUSHI_BASE_AUDIO_FRONTEND_H
#define SUSHI_BASE_AUDIO_FRONTEND_H

#include "engine/engine.h"

namespace sushi {

namespace audio_frontend {

enum class AudioFrontendInitStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_INPUT_FILE,
    INVALID_OUTPUT_FILE
};

struct BaseAudioFrontendConfiguration
{
    BaseAudioFrontendConfiguration()
    {}

    virtual ~BaseAudioFrontendConfiguration()
    {}
};

class BaseAudioFrontend
{
public:
    BaseAudioFrontend(sushi_engine::EngineBase* engine) :
            _engine(engine)
    {}

    virtual ~BaseAudioFrontend()
    {}

    virtual AudioFrontendInitStatus init(BaseAudioFrontendConfiguration* config)
    {
        _config = config;
        return AudioFrontendInitStatus::OK;
    }

    virtual void cleanup() = 0;

    virtual void run() = 0;

protected:
    BaseAudioFrontendConfiguration* _config;
    sushi_engine::EngineBase* _engine;
};


}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_BASE_AUDIO_FRONTEND_H
