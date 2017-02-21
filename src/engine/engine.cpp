#include "engine.h"

#include <iostream>

#include "logging.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"
#include "plugins/sample_player_plugin.h"


namespace sushi {
namespace engine {

MIND_GET_LOGGER;

EngineReturnStatus set_up_channel_config(PluginChain& chain, const Json::Value& mode)
{
    int chain_channel_count;
    if (mode.asString() == "mono")
    {
        chain_channel_count = 1;
    } else if (mode.asString() == "stereo")
    {
        chain_channel_count = 2;
    } else
    {
        MIND_LOG_ERROR("Unrecognized channel configuration mode \"{}\"", mode.asString());
        return EngineReturnStatus::INVALID_STOMPBOX_CHAIN;
    }
    chain.set_input_channels(chain_channel_count);
    chain.set_output_channels(chain_channel_count);
    return EngineReturnStatus::OK;
}


AudioEngine::AudioEngine(int sample_rate) : BaseEngine::BaseEngine(sample_rate)
{}

AudioEngine::~AudioEngine()
{}

int AudioEngine::n_channels(int chain)
{
    if (chain < MAX_CHAINS)
    {
        return _audio_graph[chain].input_channels();
    }
    return 0;
}

std::unique_ptr<InternalPlugin> AudioEngine::_make_stompbox_from_unique_id(const std::string &uid)
{
    InternalPlugin* instance = nullptr;

    if (uid == "sushi.testing.passthrough")
    {
        instance = new passthrough_plugin::PassthroughPlugin();
    }
    else if (uid == "sushi.testing.gain")
    {
        instance = new gain_plugin::GainPlugin();
    }
    else if (uid == "sushi.testing.equalizer")
    {
        instance = new equalizer_plugin::EqualizerPlugin();
    }
    else if (uid == "sushi.testing.sampleplayer")
    {
        instance = new sample_player_plugin::SamplePlayerPlugin();
    }

    return std::unique_ptr<InternalPlugin>(instance);
}

EngineReturnStatus AudioEngine::_fill_chain_from_json_definition(const int chain_idx,
                                                                 const Json::Value &chain_def)
{
    EngineReturnStatus status = set_up_channel_config(_audio_graph[chain_idx], chain_def["mode"]);
    if (status != EngineReturnStatus::OK)
    {
        return status;
    }
    const Json::Value &stompbox_defs = chain_def["stompboxes"];
    if (stompbox_defs.isArray())
    {
        for (const Json::Value &stompbox_def : stompbox_defs)
        {
            auto uid = stompbox_def["stompbox_uid"].asString();
            auto instance = _make_stompbox_from_unique_id(uid);
            if (instance == nullptr)
            {
                MIND_LOG_ERROR("Invalid plugin uid {} in configuration file for chain {}", uid, chain_idx);
                return EngineReturnStatus::INVALID_STOMPBOX_UID;
            }
            auto instance_id = stompbox_def["id"].asString();
            _instances_id_to_stompbox[instance_id] = std::move(instance);
            // TODO - look over ownership here - see ardours use of shared_ptr for instance
            _audio_graph[chain_idx].add(_instances_id_to_stompbox[instance_id].get());

            // TODO WIP : rivedi in base a nuovo sistema init / set sample rate
            // StompBoxConfig config;
            // config.sample_rate = _sample_rate;
            // config.controller = _instances_id_to_stompbox[instance_id].get();
            // instance->init(config);
        }
    }
    else
    {
        MIND_LOG_ERROR("Invalid format for stompbox chain n. {} in configuration file", chain_idx);
        return EngineReturnStatus::INVALID_STOMPBOX_CHAIN;
    }
    return EngineReturnStatus::OK;

}

// TODO: eventually when configuration complexity grows, move this stuff in a separate class
EngineReturnStatus AudioEngine::init_from_json_array(const Json::Value &chains)
{
    /* TODO, eventually remove the restrictions on no of channels when we
     * allow dynamically allocated chains */
    if (chains.isArray() && ((chains.size() > MAX_CHAINS) || (chains.size() == 0)))
    {
        MIND_LOG_ERROR("Incorrect number of stompbox chains ({}) in configuration file", chains.size());
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    for (int i = 0; i < static_cast<int>(chains.size()); ++i)
    {
        EngineReturnStatus ret_code = _fill_chain_from_json_definition(i, chains[i]);
        if (ret_code != EngineReturnStatus::OK)
        {
            return ret_code;
        }
    }
    return EngineReturnStatus::OK;
}


void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* Put the channels from in_buffer into the audio graph based on the graphs channel count
     * Note that its assumed that number of input and output channels are equal. */

    int start_channel = 0;
    int no_of_channels;
    for (auto graph = _audio_graph.begin(); graph != _audio_graph.end(); ++graph)
    {
        no_of_channels = (*graph).input_channels();
        if (start_channel + no_of_channels <= in_buffer->channel_count())
        {
            ChunkSampleBuffer ch_in = ChunkSampleBuffer::create_non_owning_buffer(*in_buffer,
                                                                                  start_channel,
                                                                                  no_of_channels);
            ChunkSampleBuffer ch_out = ChunkSampleBuffer::create_non_owning_buffer(*out_buffer,
                                                                                   start_channel,
                                                                                   no_of_channels);
            (*graph).process_audio(ch_in, ch_out);
            start_channel += no_of_channels;
        } else
        {
            break;
        }
    }
    if (start_channel < in_buffer->channel_count())
    {
        MIND_LOG_WARNING("Warning, not all input channels processed, {} out of {} processed", start_channel,
                         in_buffer->channel_count());
    }
}


EngineReturnStatus AudioEngine::send_rt_event(BaseEvent* event)
{
    assert(event);
    auto processor_node = _instances_id_to_stompbox.find(event->processor_id());
    if (processor_node == _instances_id_to_stompbox.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    processor_node->second->process_event(event);
    return EngineReturnStatus::OK;
}

} // namespace engine
} // namespace sushi