/**
 * @brief Base classes for audio frontends
 */
#ifndef SUSHI_BASE_AUDIO_FRONTEND_H
#define SUSHI_BASE_AUDIO_FRONTEND_H

#include "engine/engine.h"
#include "engine/midi_dispatcher.h"

namespace sushi {

namespace audio_frontend {

constexpr int MAX_FRONTEND_CHANNELS = 8;

/**
 * @brief Error codes returned from init()
 */
enum class AudioFrontendStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_INPUT_FILE,
    INVALID_OUTPUT_FILE,
    INVALID_SEQUENCER_DATA,
    INVALID_CHUNK_SIZE,
    AUDIO_HW_ERROR
};

/**
 * @brief Dummy base class to hold frontend configurations
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
    BaseAudioFrontend(engine::BaseEngine* engine, midi_dispatcher::MidiDispatcher* midi_dispatcher) :
            _engine(engine), _midi_dispatcher(midi_dispatcher)
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
    virtual AudioFrontendStatus init(BaseAudioFrontendConfiguration* config)
    {
        _config = config;
        return AudioFrontendStatus::OK;
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
    engine::BaseEngine* _engine;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};


}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_BASE_AUDIO_FRONTEND_H
