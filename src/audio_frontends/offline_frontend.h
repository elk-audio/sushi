/**
 * @brief Offline frontend to process audio files in chunks
 *
 */

#ifndef SUSHI_OFFLINE_FRONTEND_H
#define SUSHI_OFFLINE_FRONTEND_H

#include <string>
#include <vector>

#include <sndfile.h>

#include "base_audio_frontend.h"
#include "library/rt_event.h"

namespace sushi {

namespace audio_frontend {

constexpr int OFFLINE_FRONTEND_CHANNELS = 2;

struct OfflineFrontendConfiguration : public BaseAudioFrontendConfiguration
{

    OfflineFrontendConfiguration(const std::string input_filename,
                                 const std::string output_filename) :
            input_filename(input_filename),
            output_filename(output_filename)
    {}

    virtual ~OfflineFrontendConfiguration()
    {}

    std::string input_filename;
    std::string output_filename;
};

class OfflineFrontend : public BaseAudioFrontend
{
public:
    OfflineFrontend(engine::BaseEngine* engine, midi_dispatcher::MidiDispatcher* midi_dispatcher) :
            BaseAudioFrontend(engine, midi_dispatcher),
            _input_file(nullptr),
            _output_file(nullptr)
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

private:
    SNDFILE*    _input_file;
    SNDFILE*    _output_file;
    SF_INFO     _soundfile_info;
    bool        _mono;

    SampleBuffer<AUDIO_CHUNK_SIZE> _buffer{OFFLINE_FRONTEND_CHANNELS};

    std::vector<Event*> _event_queue;
};

}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_OFFLINE_FRONTEND_H
