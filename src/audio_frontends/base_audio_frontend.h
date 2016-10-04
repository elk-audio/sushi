/**
 * @brief Base classes for audio frontends
 */
#ifndef SUSHI_BASE_AUDIO_FRONTEND_H
#define SUSHI_BASE_AUDIO_FRONTEND_H

#include "engine/engine.h"

namespace sushi {

namespace audio_frontend {

static constexpr unsigned int MAX_SAMPLE_RATE = 3072000; // 48000 * 64
static constexpr unsigned int MAX_N_CHANNELS = 4096;

enum class AudioFrontendInitStatus
{
    OK,
    INVALID_SAMPLE_RATE,
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
