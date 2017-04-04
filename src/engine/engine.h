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

#include "EASTL/vector.h"
#include <json/json.h>

#include "plugin_chain.h"
#include "library/sample_buffer.h"
#include "library/mind_allocator.h"
#include "library/internal_plugin.h"
#include "library/midi_decoder.h"
#include "engine/midi_dispatcher.h"

// TODO: this not needed anymore, remove it
class StompBox;

namespace sushi {
namespace engine {

enum class EngineReturnStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_STOMPBOX_UID,
    INVALID_PARAMETER_UID,
    INVALID_STOMPBOX_CHAIN,
    INVALID_ARGUMENTS
};

class BaseEngine
{
public:
    BaseEngine(int sample_rate) : _sample_rate(sample_rate)
    {}

    virtual ~BaseEngine()
    {}

    int sample_rate()
    {
        return _sample_rate;
    }

    virtual void set_audio_input_channels(int channels)
    {
        _audio_inputs = channels;
    }

    virtual void set_audio_output_channels(int channels)
    {
        _audio_outputs = channels;
    }

    virtual void set_midi_input_ports(int channels)
    {
        _midi_inputs = channels;
    }

    virtual void set_midi_output_ports(int channels)
    {
        _midi_outputs = channels;
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

    virtual EngineReturnStatus connect_midi_cc_data(int /*midi_port*/,
                                                    int /*cc_no*/,
                                                    const std::string & /*processor_id*/,
                                                    const std::string & /*parameter*/,
                                                    float /*min_range*/,
                                                    float /*max_range*/,
                                                    int /*midi_channel*/)
    {
        return EngineReturnStatus::OK;
    }

    virtual EngineReturnStatus connect_midi_kb_data(int /*midi_port*/,
                                                    const std::string& /*chain_id*/,
                                                    int /*midi_channel*/)
    {
        return EngineReturnStatus::OK;
    }


    virtual int n_channels_in_chain(int /*chain*/)
    {
        return 2;
    }

    virtual void process_midi(int /*input*/, int /*offset*/, const uint8_t* /*data*/, size_t /*size*/) {};

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;

    virtual EngineReturnStatus send_rt_event(BaseEvent* event) = 0;

protected:
    int _sample_rate;
    int _audio_inputs{0};
    int _audio_outputs{0};
    int _midi_inputs{0};
    int _midi_outputs{0};
};


class AudioEngine : public BaseEngine
{
public:
    AudioEngine(int sample_rate);

     ~AudioEngine();

    /**
     * @brief Connect an audio input to a chain.
     */
    EngineReturnStatus connect_audio_mono_input(int channel, const std::string& chain_id) override;

    /**
     * @brief Connect a chain to an audio output.
     */
    EngineReturnStatus connect_audio_mono_output(int channel, const std::string& chain_id) override;

    /**
     * @brief Connect 2 audio inputs to a stereo chain.
     */
    EngineReturnStatus connect_audio_stereo_input(int left_channel_idx,
                                                  int right_channel_idx,
                                                  const std::string& chain_id) override;

    /**
     * @brief Connect a stereo chain to 2 audio outputs.
     */
    EngineReturnStatus connect_audio_stereo_output(int left_channel_idx,
                                                   int right_channel_idx,
                                                   const std::string& chain_id) override;

    /**
     * @brief Connect midi cc data to a named parameter on a processor
     */
    EngineReturnStatus connect_midi_cc_data(int midi_port,
                                            int cc_no,
                                            const std::string &processor_id,
                                            const std::string &parameter,
                                            float min_range,
                                            float max_range,
                                            int midi_channel = midi::MidiChannel::OMNI) override;

    /**
     * @brief Connect midi keyboard data to a specific chain
     */
    EngineReturnStatus connect_midi_kb_data(int midi_port,
                                            const std::string& chain_id,
                                            int midi_channel = midi::MidiChannel::OMNI) override;

    /**
     * @brief Return the number of configured channels for a specific processing chainn
     *
     * @param chain The index to the chain
     * @return Number of channels the chain is configured to use.
     */
    int n_channels_in_chain(int chain) override;

    /**
     * @brief Statically initialize engine and stompbox processing chains from definition in a JSON file
     * @param stompboxes_defs path to configuration file
     * @return EngineInitStatus::OK in case of success,
     *         different error code otherwise
     */
    EngineReturnStatus init_chains_from_json_array(const Json::Value &chains);

    /**
     * @brief Statically initialize midi connetions to chains and midi cc mappings to parameters.
     * @param Path to configuration file
     * @return EngineInitStatus::OK in case of success,
     *         different error code otherwise
     */
    EngineReturnStatus init_midi_from_json_array(const Json::Value &connections_def);


    void process_midi(int input, int offset, const uint8_t* data, size_t size) override
    {
        _midi_dispatcher.process_midi(input, offset, data, size);
    }

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

    EngineReturnStatus send_rt_event(BaseEvent* event) override;

protected:
    eastl::vector<PluginChain*> _audio_graph;

private:
    /**
     * @brief Instantiate a plugin instance of a given type
     * @param uid String unique id
     * @return Pointer to plugin instance if uid is valid,
     *         nullptr otherwise
     */
    std::unique_ptr<Processor> _make_stompbox_from_unique_id(const std::string &uid);

    /**
     * @brief Instantiate plugins and chain from a JSON chain definition.
     * @param chain_def JSON node with list of stompboxes for this chain
     * @return EngineInitStatus::OK if all plugins in the definitions are instantiated correctly,
     *         different error code otherwise
     */
    EngineReturnStatus _fill_chain_from_json_definition(const Json::Value &stompbox_def);

    // TODO: eventually port to EASTL
    // Owns all processors, including plugin chains
    std::map<std::string, std::unique_ptr<Processor>> _instances_id_to_processors;

    midi_dispatcher::MidiDispatcher _midi_dispatcher{this};
};

/**
 * @brief Freestanding function to handle stereo/mono setup from json.
 */
EngineReturnStatus set_up_channel_config(PluginChain& chain, const Json::Value& mode);

/**
 * @brief Freestanding helper function.
 */
int get_midi_channel_from_json(const Json::Value& value);
} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
