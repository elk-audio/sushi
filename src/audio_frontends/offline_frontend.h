/**
 * @brief Offline frontend to process audio files in chunks
 *
 */

#ifndef SUSHI_OFFLINE_FRONTEND_H
#define SUSHI_OFFLINE_FRONTEND_H

#include "base_audio_frontend.h"

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

    // FIXME: quick workaround to implement event processing before having defined
    //        an Event class. Change in the future when that will be available.
    std::vector<std::tuple<int, std::string, std::string, float>> _event_queue;
};

}; // end namespace audio_frontend

}; // end namespace sushi

#endif //SUSHI_OFFLINE_FRONTEND_H
