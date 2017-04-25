/**
 * @brief Frontend using Xenomai rt framework
 *
 */

#ifndef SUSHI_XENOMAI_FRONTEND_H
#define SUSHI_XENOMAI_FRONTEND_H

#ifdef SUSHI_BUILD_WITH_XENOMAI

#include "base_audio_frontend.h"
#include "library/plugin_events.h"
#include "library/event_fifo.h"
#include "library/circular_fifo.h"
#include "library/circularfifo_memory_relaxed_aquire_release.h"
#include "control_frontends/osc_frontend.h"

#include <string>
#include <tuple>
#include <vector>
#include <thread>

#include <json/json.h>
#include <sndfile.h>

#include <task.h>
#include <timer.h>
#include <xenomai/init.h>

namespace sushi {

namespace audio_frontend {

// Audio buffer contains ~3 s of audio @ 44 kHz
typedef memory_relaxed_aquire_release::CircularFifo<ChunkSampleBuffer, 2000> AudioQueue;

constexpr auto DISK_IO_PERIODICITY = std::chrono::seconds(1);

/**
 * @brief Helper class to provide asynchronous, faux realtime i/o from disk
 *        instead of a sound interface.
 */
class DiskIoHandler
{
public:
    DiskIoHandler(AudioQueue* in_queue, AudioQueue* out_queue) : _in_queue(in_queue),
                                                                 _out_queue(out_queue) {}
    ~DiskIoHandler();

    AudioFrontendStatus init(const std::string& input_filename, const std::string& output_filename);

    /**
     * @brief Get the samplerate of the current file stream.
     */
    int samplerate() {return _soundfile_info.samplerate;}

    /**
     * @brief Get the number of channels of the current file stream.
     */
    int channels() {return _soundfile_info.channels;}

    /**
     * @brief Start the streaming.
     * @return true if succesfully started the stream.
     */
    bool run();

    /**
     * @brief Stop the streaming.
     */
    void stop();

private:
    void worker();

    std::atomic_bool _running{false};
    std::thread _io_thread;
    AudioQueue* _in_queue;
    AudioQueue* _out_queue;

    SNDFILE*    _input_file{nullptr};
    SNDFILE*    _output_file{nullptr};
    SF_INFO     _soundfile_info;

    float*  _in_file_buffer{nullptr};
    float*  _out_file_buffer{nullptr};
};


struct XenomaiFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiFrontendConfiguration(const std::string input_file,
                                 const std::string output_file) : input_file(input_file),
                                                                  output_file(output_file)
    {}

    virtual ~XenomaiFrontendConfiguration()
    {}

    std::string input_file;
    std::string output_file;
};


class XenomaiFrontend : public BaseAudioFrontend
{
public:
    XenomaiFrontend(engine::BaseEngine* engine) : BaseAudioFrontend(engine) {}

    virtual ~XenomaiFrontend()
    {
        cleanup();
    }

    /**
     * @brief The realtime process callback given to jack and which will be
     *        called for every processing chunk.
     * @param nframes Number of frames in this processing chunk.
     * @param arg In this case is a pointer to the XenomaiFrontend instance.
     * @return
     */
    int rt_process_callback()
    {
        return this->internal_process_callback();
    }

    /**
     * @brief Initialize the frontend and setup Jack client.
     * @param config Configuration struct
     * @return OK on successful initialization, error otherwise.
     */
    AudioFrontendStatus init(BaseAudioFrontendConfiguration* config) override;

    /**
     * @brief Parse timestamped events from JSON structure and put them into an internal queueu.
     * @param events
     * @return AudioFrontendStatus::OK if successful, other error code otherwise
     */
    AudioFrontendStatus add_sequencer_events_from_json_def(const Json::Value& events);

    /**
     * @brief Call to clean up resources and release ports
     */
    void cleanup() override;

    /**
     * @brief Activate the realtime frontend, currently blocking.
     */
    void run() override;

private:
    /* Internal process callback function */
    int internal_process_callback();

    SampleBuffer<AUDIO_CHUNK_SIZE> _out_buffer{2};
    SampleBuffer<AUDIO_CHUNK_SIZE> _in_buffer{2};

    AudioQueue _in_audio_queue{_in_buffer};
    AudioQueue _out_audio_queue{_in_buffer};

    DiskIoHandler _disk_io{&_out_audio_queue, &_in_audio_queue};

    // TODO - Not really a queue in offline mode, just a list of events sorted by time
    std::vector<std::tuple<int, BaseEvent*>> _event_queue;

    int64_t _samplecount{0};
    //std::unique_ptr<control_frontend::OSCFrontend> _osc_control;
};

/**
 * @brief Xenomai task function. Used to generate a realtime callback for the frontend.
 */
void xenomai_callback_generator(void* data);

}; // end namespace jack_frontend
}; // end namespace sushi

#endif //SUSHI_BUILD_WITH_XENOMAI
#ifndef SUSHI_BUILD_WITH_XENOMAI
/* If Xenomai is disabled in the build, the xenomai frontend is replaced with this
   dummy frontend whose only purpose is to assert if you try to use it */

#include "base_audio_frontend.h"
namespace sushi {
namespace audio_frontend {
struct XenomaiFrontendConfiguration : public BaseAudioFrontendConfiguration
{
    XenomaiFrontendConfiguration(const std::string,
                              const std::string) {}
};

class XenomaiFrontend : public BaseAudioFrontend
{
public:
    XenomaiFrontend(engine::BaseEngine* engine);
    AudioFrontendStatus init(BaseAudioFrontendConfiguration*) override {return AudioFrontendStatus::OK;}
    AudioFrontendStatus add_sequencer_events_from_json_def(const Json::Value&) {return AudioFrontendStatus::OK;}
    void cleanup() override {}
    void run() override {}
};
}; // end namespace audio_frontend
}; // end namespace sushi
#endif

#endif //SUSHI_XENOMAI_FRONTEND_H
