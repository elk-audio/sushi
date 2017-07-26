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

#include "EASTL/vector.h"

#include "plugin_chain.h"
#include "engine/receiver.h"
#include "library/sample_buffer.h"
#include "library/mind_allocator.h"
#include "library/internal_plugin.h"
#include "library/midi_decoder.h"
#include "library/event_fifo.h"


namespace sushi {
namespace engine {

constexpr int PROC_ID_ARRAY_INCREMENT = 100;

enum class EngineReturnStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_PLUGIN_UID,
    INVALID_PLUGIN_NAME,
    INVALID_PLUGIN_TYPE,
    INVALID_PROCESSOR,
    INVALID_PARAMETER,
    INVALID_PLUGIN_CHAIN,
    QUEUE_FULL
};

enum class PluginType
{
    INTERNAL,
    VST2X,
    VST3X
};

enum class StreamingState
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

    int sample_rate()
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

    virtual EngineReturnStatus connect_audio_mono_input(int /*channel*/, const std::string& /*chain_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_mono_output(int /*channel*/, const std::string& /*chain_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_stereo_input(int /*left_channel_idx*/,
                                                          int /*right_channel_idx*/,
                                                          const std::string& /*chain_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_audio_stereo_output(int /*left_channel_idx*/,
                                                           int /*right_channel_idx*/,
                                                           const std::string& /*chain_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual int n_channels_in_chain(int /*chain*/)
    {
        return 2;
    }

    virtual bool realtime()
    {
        return true;
    }

    virtual void enable_realtime(bool /*enabled*/) {}

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;

    virtual EngineReturnStatus send_rt_event(Event event) = 0;

    virtual EngineReturnStatus send_async_event(const Event &event) = 0;


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

    virtual EngineReturnStatus create_plugin_chain(const std::string& /*chain_id*/, int /*chain_channel_count*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus delete_plugin_chain(const std::string& /*chain_id*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus add_plugin_to_chain(const std::string& /*chain_id*/,
                                                   const std::string& /*uid*/,
                                                   const std::string& /*name*/,
                                                   const std::string& /*file*/,
                                                   PluginType /*plugin_type*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus remove_plugin_from_chain(const std::string& /*chain_id*/,
                                                        const std::string& /*plugin_id*/)
    {
        return EngineReturnStatus::OK;
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
    virtual void set_sample_rate(float sample_rate);

    /**
     * @brief Return the number of configured channels for a specific processing chainn
     *
     * @param chain The index to the chain
     * @return Number of channels the chain is configured to use.
     */
    int n_channels_in_chain(int chain) override;

    virtual bool realtime() override;

    /**
     * @brief Set the engine to operate in realtime mode. In this mode process_chunk and
     *        send_rt_event is assumed to be called from a realtime thread.
     *        All other function calls are assumed to be made from non-realtime threads
     *
     * @param enabled true to enable realtime mode, false to disable
     */
    virtual void enable_realtime(bool enabled) override;

    /**
     * @brief Process one chunk of audio from in_buffer and write the result to out_buffer
     * @param in_buffer input audio buffer
     * @param out_buffer output audio buffer
     */
    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

    /**
     * @brief Process an event directly. In a realtime processing setup this must be
     *        called from the realtime thread before calling process_chunk()
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_rt_event(Event event) override;

    /**
     * @brief Called from a non-realtime thread to process an event in the realtime
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_async_event(const Event &event) override;
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
     * @brief Creates an empty plugin chain
     * @param chain_id The unique id of the chain to be created.
     * @param chain_channel_count The number of channels in the plugin chain.
     * @return EngineInitStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus create_plugin_chain(const std::string& chain_id, int chain_channel_count) override;

    /**
     * @brief Delete a chain, currently assumes that the chain is empty before calling
     * @param chain_id The unique name of the chain to delete
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    virtual EngineReturnStatus delete_plugin_chain(const std::string& chain_id) override;

    /**
     * @brief Creates and adds a plugin to a chain.
     * @param chain_id The unique id of the chain to which the processor will be appended
     * @param uid The unique id of the plugin
     * @param name The name to give the plugin after loading
     * @param plugin_path The file to load the plugin from, only valid for external plugins
     * @param plugin_type The type of plugin, i.e. internal or external
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus add_plugin_to_chain(const std::string& chain_id,
                                           const std::string& uid,
                                           const std::string& name,
                                           const std::string& plugin_path,
                                           PluginType plugin_type) override;

    /**
     * @brief Remove a given plugin from a chain and delete it
     * @param chain_id The unique name of the chain the contains the plugin
     * @param plugin_id The unique name of the plugin
     * @return EngineReturnStatus::OK in case of success, different error code otherwise
     */
    virtual EngineReturnStatus remove_plugin_from_chain(const std::string& chain_id,
                                                        const std::string& plugin_id) override;

protected:
    std::vector<PluginChain*> _audio_graph;

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
    bool _insert_processor_in_processing_part(Processor* processor);

    /**
     * @brief Remove a processor from the realtime processing part
     *        Must be called before de-registering a processor.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    bool _remove_processor_from_processing_part(ObjectId processor);

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
    bool _handle_internal_events(Event &event);

    // Owns all processors, including plugin chains
    std::map<std::string, std::unique_ptr<Processor>> _processors_by_unique_name;
    // Processors indexed by their unique 32 bit id
    std::vector<Processor*> _processors_by_unique_id{PROC_ID_ARRAY_INCREMENT, nullptr};

    std::atomic<StreamingState> _state{StreamingState::STOPPED};

    EventFifo _control_queue_in;
    EventFifo _control_queue_out;
    receiver::AsynchronousEventReceiver _event_receiver{&_control_queue_out};
};

/**
 * @brief Helper function to encapsulate state changes from transient states
 * @param current_state The current state of the engine
 * @return A new, non-transient state
 */
StreamingState update_state(StreamingState current_state);

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
