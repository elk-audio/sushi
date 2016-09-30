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

// TODO: rename public fields without leading _
struct BaseAudioFrontendConfiguration
{
    BaseAudioFrontendConfiguration(const unsigned int sample_rate,
                                   const unsigned int n_channels) :
            _sample_rate(sample_rate),
            _n_channels(n_channels)
    {}

    virtual ~BaseAudioFrontendConfiguration()
    {}

    unsigned int _sample_rate;
    unsigned int _n_channels;
};

class BaseAudioFrontend
{
public:
    BaseAudioFrontend()
    {}

    virtual ~BaseAudioFrontend()
    {
        delete _engine;
    }

    virtual AudioFrontendInitStatus init(const BaseAudioFrontendConfiguration* config)
    {
        _config = config;

        if (   (config->_sample_rate < 1)
            || (config->_sample_rate > MAX_SAMPLE_RATE)
            || ( ((config->_sample_rate) % 2) != 0)     )
        {
            return AudioFrontendInitStatus::INVALID_SAMPLE_RATE;
        }

        if (   (config->_n_channels < 1)
            || (config->_n_channels > MAX_N_CHANNELS)  )
        {
            return AudioFrontendInitStatus::INVALID_N_CHANNELS;
        }

        _engine = new sushi_engine::SushiEngine(_config->_sample_rate);

        return AudioFrontendInitStatus::OK;

    }

    virtual void cleanup();

    virtual void run();

protected:
    BaseAudioFrontendConfiguration* _config;
    sushi_engine::SushiEngine* _engine;
};


}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_BASE_AUDIO_FRONTEND_H
