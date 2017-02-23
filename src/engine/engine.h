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

class StompBox;

namespace sushi {
namespace engine {

/* Maximum number of parallell chains, chains are not dynamically allocated yet */
constexpr int MAX_CHAINS = 2;

enum class EngineReturnStatus
{
    OK,
    INVALID_N_CHANNELS,
    INVALID_STOMPBOX_UID,
    INVALID_PARAMETER_UID,
    INVALID_STOMPBOX_CHAIN
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

    virtual int n_channels(int /*chain*/)
    {
        return 2;
    }

    virtual void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;

    virtual EngineReturnStatus send_rt_event(BaseEvent* event) = 0;

protected:
    int _sample_rate;
};


class AudioEngine : public BaseEngine
{
public:
    AudioEngine(int sample_rate);

    ~AudioEngine();

    /**
     * @brief Return the number of configured channels for a specific processing chain
     * @param chain The index to the chain
     * @return Number of channels the chain is configured to use.
     */
    int n_channels(int chain) override;
    
    /**
     * @brief Statically initialize engine and stompbox processing chains from definition in a JSON file
     * @param stompboxes_defs path to configuration file
     * @return EngineInitStatus::OK in case of success,
     *         different error code otherwise
     */
    EngineReturnStatus init_from_json_array(const Json::Value &stompboxes_defs);

    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) override;

    EngineReturnStatus send_rt_event(BaseEvent* event) override;

protected:
    eastl::vector<PluginChain> _audio_graph{MAX_CHAINS};

private:
    /**
     * @brief Instantiate a plugin instance of a given type
     * @param uid String unique id
     * @return Pointer to plugin instance if uid is valid,
     *         nullptr otherwise
     */
    std::unique_ptr<InternalPlugin> _make_stompbox_from_unique_id(const std::string &uid);

    /**
     * @brief Instantiate plugins from a JSON chain definition and put them in given chain.
     * @param chain_idx Chain index in internal graph
     * @param chain_def JSON node with list of stompboxes for this chain
     * @return EngineInitStatus::OK if all plugins in the definitions are instantiated correctly,
     *         different error code otherwise
     */
    EngineReturnStatus _fill_chain_from_json_definition(const int chain_idx,
                                                        const Json::Value &stompbox_def);

    // TODO: eventually port to EASTL
    std::map<std::string, std::unique_ptr<InternalPlugin>> _instances_id_to_stompbox;
};

/**
 * @brief Freestanding function to handle stereo/mono setup from json.
 */
EngineReturnStatus set_up_channel_config(PluginChain& chain, const Json::Value& mode);

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
