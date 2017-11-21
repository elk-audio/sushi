/**
 * @Brief real time audio processing engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_ENGINE_H
#define SUSHI_ENGINE_H

#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <mutex>

#include "EASTL/vector.h"

#include "engine/event_dispatcher.h"
#include "track.h"
#include "engine/receiver.h"
#include "engine/transport.h"
#include "library/sample_buffer.h"
#include "library/mind_allocator.h"
#include "library/internal_plugin.h"
#include "library/midi_decoder.h"
#include "library/event_fifo.h"
#include "library/types.h"


namespace sushi {
namespace engine {

constexpr int MAX_RT_PROCESSOR_ID = 1000;


enum class EngineReturnStatus
{
    OK,
    ERROR,
    INVALID_N_CHANNELS,
    INVALID_PLUGIN_UID,
    INVALID_PLUGIN_NAME,
    INVALID_PLUGIN_TYPE,
    INVALID_PROCESSOR,
    INVALID_PARAMETER,
    INVALID_TRACK,
    QUEUE_FULL
};

enum class PluginType
{
    INTERNAL,
    VST2X,
    VST3X
};

enum class RealtimeState
{
    STARTING,
    RUNNING,
    STOPPING,
    STOPPED
};

class BaseEngine
{
public:
    BaseEngine(float sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~BaseEngine()
    {}

    float sample_rate()
    {
        return _sample_rate;
    }

    virtual void set_sample_rate(float sample_rate)
    {
        _sample_rate = sample_rate;
    }

    virtual void set_audio_input_channels(int channels)
    {
        _audio_inputs = channels;
    }

    virtual void set_audio_output_channels(int channels)
    {
        _audio_outputs = channels;
    }

    virtual EngineReturnStatus connect_audio_mono_input(int /*channel*/, const std::string& /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_mono_output(int /*channel*/, const std::string& /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_stereo_input(int /*left_channel_idx*/,
                                                          int /*right_channel_idx*/,
                                                          const std::string& /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_stereo_output(int /*left_channel_idx*/,
                                                           int /*right_channel_idx*/,
                                                           const std::string& /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual int n_channels_in_track(int /*track_no*/)
    {
        return 2;
    }

    virtual bool realtime()
    {
        return true;
    }

    virtual void enable_realtime(bool /*enabled*/) {}

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;

    virtual void update_time(int64_t /*usec*/, int64_t /*samples*/) = 0;

    virtual EngineReturnStatus send_rt_event(RtEvent& event) = 0;

    virtual EngineReturnStatus send_async_event(RtEvent& event) = 0;


    virtual std::pair<EngineReturnStatus, ObjectId> processor_id_from_name(const std::string& /*name*/)
    {
        return std::make_pair(EngineReturnStatus::OK, 0);
    };

    virtual std::pair<EngineReturnStatus, ObjectId> parameter_id_from_name(const std::string& /*processor_name*/,
                                                                           const std::string& /*parameter_name*/)
    {
        return std::make_pair(EngineReturnStatus::OK, 0);
    };

    virtual std::pair<EngineReturnStatus, const std::string> processor_name_from_id(const ObjectId /*id*/)
    {
        return std::make_pair(EngineReturnStatus::OK, "");
    };

    virtual std::pair<EngineReturnStatus, const std::string> parameter_name_from_id(const std::string& /*processor_name*/,
                                                                                    const ObjectId /*id*/)
    {
        return std::make_pair(EngineReturnStatus::OK, "");
    };

    virtual EngineReturnStatus create_track(const std::string & /*track_id*/, int /*channel_count*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus delete_track(const std::string & /*track_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus add_plugin_to_track(const std::string & /*track_id*/,
                                                   const std::string & /*uid*/,
                                                   const std::string & /*name*/,
                                                   const std::string & /*file*/,
                                                   PluginType /*plugin_type*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus remove_plugin_from_track(const std::string & /*track_id*/,
                                                        const std::string & /*plugin_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual const std::map<std::string, std::unique_ptr<Processor>>& all_processors()
    {
        static std::map<std::string, std::unique_ptr<Processor>> tmp;
        return tmp;
    }

    virtual const std::vector<Track*>& all_tracks()
    {
        static std::vector<Track*> tmp;
        return tmp;
    }

    virtual sushi::dispatcher::BaseEventDispatcher* event_dispatcher()
    {
        return nullptr;
    }

protected:
    float _sample_rate;
    int _audio_inputs{0};
    int _audio_outputs{0};
};


class AudioEngine : public BaseEngine
{
public:
    AudioEngine(float sample_rate);

     ~AudioEngine();

    /**
     * @brief Configure the Engine with a new samplerate.
     * @param sample_rate The new sample rate in Hz
     */
    void set_sample_rate(float sample_rate) override;

    /**
     * @brief Return the number of configured channels for a specific track
     *
     * @param track The index to the track
     * @return Number of channels the track is configured to use.
     */
    int n_channels_in_track(int track) override;

    /**
     * @brief Returns whether the engine is running in a realtime mode or not
     * @return true if the engine is currently processing in realtime mode, false otherwise
     */
    bool realtime() override;

    /**
     * @brief Set the engine to operate in realtime mode. In this mode process_chunk and
     *        send_rt_event is assumed to be called from a realtime thread.
     *        All other function calls are assumed to be made from non-realtime threads
     *
     * @param enabled true to enable realtime mode, false to disable
     */
    void enable_realtime(bool enabled) override;

    /**
     * @brief Process one chunk of audio from in_buffer and write the result to out_buffer
     * @param in_buffer input audio buffer
     * @param out_buffer output audio buffer
     */
    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

    /**
     * @brief Set the current time for the start of the current audio chunk
     * @param timestamp Current time in microseconds
     * @param samples Current number of samples processed
     */
    void update_time(MicroTime timestamp, int64_t samples)
    {
        _transport.set_time(timestamp, samples);
    }

    /**
     * @brief Process an event directly. In a realtime processing setup this must be
     *        called from the realtime thread before calling process_chunk()
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_rt_event(RtEvent& event) override;

    /**
     * @brief Called from a non-realtime thread to process an event in the realtime
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_async_event(RtEvent& event) override;
    /**
     * @brief Get the unique id of a processor given its name
     * @param unique_name The unique name of a processor
     * @return the unique id of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, ObjectId> processor_id_from_name(const std::string& name) override;

    /**
     * @brief Get the unique (per processor) id of a parameter.
     * @param processor_name The unique name of a processor
     * @param The unique name of a parameter of the above processor
     * @return the unique id of the parameter, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, ObjectId> parameter_id_from_name(const std::string& /*processor_name*/,
                                                                   const std::string& /*parameter_name*/);

    /**
     * @brief Get the unique name of a processor of a known unique id
     * @param uid The unique id of the processor
     * @return The name of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, const std::string> processor_name_from_id(const ObjectId uid) override;

    /**
     * @brief Get the unique name of a parameter with a known unique id
     * @param processor_name The unique name of the processor
     * @param id The unique id of the parameter to lookup.
     * @return The name of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, const std::string> parameter_name_from_id(const std::string& processor_name,
                                                                            const ObjectId id) override;
    /**
     * @brief Creates an empty track
     * @param track_id The unique id of the track to be created.
     * @param channel_count The number of channels in the track.
     * @return EngineInitStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus create_track(const std::string &track_id, int channel_count) override;

    /**
     * @brief Delete a track, currently assumes that the track is empty before calling
     * @param track_name The unique name of the track to delete
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus delete_track(const std::string &track_name) override;

    /**
     * @brief Creates and adds a plugin to a track.
     * @param track_id The unique id of the track to which the processor will be appended
     * @param uid The unique id of the plugin
     * @param name The name to give the plugin after loading
     * @param plugin_path The file to load the plugin from, only valid for external plugins
     * @param plugin_type The type of plugin, i.e. internal or external
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus add_plugin_to_track(const std::string &track_name,
                                           const std::string &uid,
                                           const std::string &name,
                                           const std::string &plugin_path,
                                           PluginType plugin_type) override;

    /**
     * @brief Remove a given plugin from a track and delete it
     * @param track_name The unique name of the track that contains the plugin
     * @param plugin_name The unique name of the plugin
     * @return EngineReturnStatus::OK in case of success, different error code otherwise
     */
    EngineReturnStatus remove_plugin_from_track(const std::string &track_name,
                                                const std::string &plugin_name) override;
    /**
     * @brief Return all processors. Potentially dangerous so use with care and eventually
     *        there should be better and safer ways of accessing processors.
     * @return An std::map containing all registered processors.
     */
    const std::map<std::string, std::unique_ptr<Processor>>& all_processors() override
    {
        return _processors;
    };

    /**
     * @brief Return all tracks. Potentially unsafe so use with care. Should
     *        eventually be replaces with a better way of accessing tracks/processors
     *        from outside the engine.
     * @return An std::vector of containing all Tracks
     */
    const std::vector<Track*>& all_tracks()
    {
        return _audio_graph;
    }

    sushi::dispatcher::BaseEventDispatcher* event_dispatcher() override
    {
        return &_event_dispatcher;
    }

private:
    /**
     * @brief Instantiate a plugin instance of a given type
     * @param uid String unique id
     * @return Pointer to plugin instance if uid is valid, nullptr otherwise
     */
    Processor* _make_internal_plugin(const std::string& uid);

    /**
     * @brief Register a newly created processor in all lookup containers
     *        and take ownership of it.
     * @param processor Processor to register
     */
    EngineReturnStatus _register_processor(Processor* processor, const std::string& name);

    /**
     * @breif Remove a processor from the engine and delete it.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    EngineReturnStatus _deregister_processor(const std::string& name);

    /**
     * @brief Add a registered processor to the realtime processing part.
     *        Must be called before a processor can be used to process audio.
     * @param processor Processor to insert
     */
    bool _insert_processor_in_realtime_part(Processor* processor);

    /**
     * @brief Remove a processor from the realtime processing part
     *        Must be called before de-registering a processor.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    bool _remove_processor_from_realtime_part(ObjectId processor);

    /**
     * @brief Checks whether a processor exists in the engine.
     * @param processor_name The unique name of the processor.
     * @return Returns true if exists, false if it does not.
     */
    bool _processor_exists(const std::string& processor_name);

    /**
     * @brief Checks whether a processor exists in the engine.
     * @param uid The unique id of the processor.
     * @return Returns true if exists, false if it does not.
     */
    bool _processor_exists(ObjectId uid);

    /**
     * @brief Process events that are to be handles by the engine directly and
     *        not by a particular processor.
     * @param event The event to handle
     * @return true if handled, false if not an engine event
     */
    bool _handle_internal_events(RtEvent &event);

    std::vector<Track*> _audio_graph;

    // All registered processors indexed by their unique name
    std::map<std::string, std::unique_ptr<Processor>> _processors;

    // Processors in the realtime part indexed by their unique 32 bit id
    // Only to be accessed from the process callback in rt mode.
    std::vector<Processor*> _realtime_processors{MAX_RT_PROCESSOR_ID, nullptr};

    std::atomic<RealtimeState> _state{RealtimeState::STOPPED};

    RtEventFifo _internal_control_queue;
    RtEventFifo _main_in_queue;
    RtEventFifo _main_out_queue;
    RtEventFifo _control_queue_out;
    std::mutex _in_queue_lock;
    receiver::AsynchronousEventReceiver _event_receiver{&_control_queue_out};
    Transport _transport;

    dispatcher::EventDispatcher _event_dispatcher{this, &_main_out_queue, &_main_in_queue};
};

/**
 * @brief Helper function to encapsulate state changes from transient states
 * @param current_state The current state of the engine
 * @return A new, non-transient state
 */
RealtimeState update_state(RealtimeState current_state);

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
