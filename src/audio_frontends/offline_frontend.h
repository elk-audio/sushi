/**
 * @brief Offline frontend to process audio files in chunks
 *
 */

#ifndef SUSHI_OFFLINE_FRONTEND_H
#define SUSHI_OFFLINE_FRONTEND_H

#include "base_audio_frontend.h"
#include "library/plugin_events.h"

#include <string>
#include <tuple>
#include <vector>

#include <json/json.h>
#include <sndfile.h>

namespace sushi {

namespace audio_frontend {


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
    OfflineFrontend(engine::BaseEngine* engine) :
            BaseAudioFrontend(engine),
            _input_file(nullptr),
            _output_file(nullptr),
            _file_buffer(nullptr)
    {}

    virtual ~OfflineFrontend()
    {
        cleanup();
    }

    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Parse timestamped events from JSON structure and put them into an internal queueu.
     * @param events
     * @return AudioFrontendStatus::OK if successful, other error code otherwise
     */
    AudioFrontendStatus add_sequencer_events_from_json_def(const Json::Value& events);

    void cleanup() override;

    void run() override;

private:
    SNDFILE*    _input_file;
    SNDFILE*    _output_file;
    SF_INFO     _soundfile_info;

    SampleBuffer<AUDIO_CHUNK_SIZE> _buffer{2};
    float*  _file_buffer;

    // TODO - Not really a queue in offline mode, just a list of events sorted by time
    std::vector<std::tuple<int, Event>> _event_queue;
};

}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_OFFLINE_FRONTEND_H
