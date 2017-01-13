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


AudioEngine::AudioEngine(int sample_rate) : BaseEngine::BaseEngine(sample_rate)
{
}

AudioEngine::~AudioEngine()
{
}

StompBox* AudioEngine::_make_stompbox_from_unique_id(const std::string &uid)
{
    StompBox* instance = nullptr;
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

    return instance;
}

EngineReturnStatus AudioEngine::_fill_chain_from_json_definition(const int chain_idx,
                                                                 const Json::Value &stompbox_defs)
{
    if (stompbox_defs.isArray())
    {
        for(const Json::Value& stompbox_def : stompbox_defs)
        {
            auto uid = stompbox_def["stompbox_uid"].asString();
            auto instance = _make_stompbox_from_unique_id(uid);
            if (instance == nullptr)
            {
                MIND_LOG_ERROR("Invalid plugin uid {} in configuration file for chain {}", uid, chain_idx);
                return EngineReturnStatus::INVALID_STOMPBOX_UID;
            }
            _audio_graph[chain_idx].add(instance);
            auto instance_id = stompbox_def["id"].asString();
            _instances_id_to_stompbox[instance_id] = std::make_unique<StompBoxManager>(instance);

            StompBoxConfig config;
            config.sample_rate = _sample_rate;
            config.controller = _instances_id_to_stompbox[instance_id].get();
            instance->init(config);
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
    // Temp workaround: verify that the given JSON has only two independent chains
    if (! (chains.isArray() && (chains.size() == MAX_CHANNELS) ) )
    {
        MIND_LOG_ERROR("Wrong number of stompbox chains in configuration file");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }

    EngineReturnStatus ret_code = _fill_chain_from_json_definition(LEFT, chains[LEFT]["stompboxes"]);
    if (ret_code != EngineReturnStatus::OK)
    {
        return ret_code;
    }
    ret_code = _fill_chain_from_json_definition(RIGHT, chains[RIGHT]["stompboxes"]);
    if (ret_code != EngineReturnStatus::OK)
    {
        return ret_code;
    }

    return EngineReturnStatus::OK;

}


void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* For now, process every channel in in_buffer separately and copy the result to
     * the corresponding channel in out_buffer */

    for (int ch = 0; ch < in_buffer->channel_count(); ++ch)
    {
        if (ch >= static_cast<int>(_audio_graph.size()) || ch >= out_buffer->channel_count())
        {
            MIND_LOG_WARNING("Warning, not all input channels processed, {} out of {} processed", ch, in_buffer->channel_count());
            break;
        }
        _tmp_bfr_in.replace(0, ch, *in_buffer);
        _audio_graph[ch].process(_tmp_bfr_in, _tmp_bfr_out);
        out_buffer->replace(ch, 0, _tmp_bfr_out);
    }
}

// FIXME: temp implementation until PluginInterface is not complete
EngineReturnStatus AudioEngine::set_stompbox_parameter(const std::string &instance_id, const std::string &param_id,
                                                       const float value)
{
    auto stompbox_instance = _instances_id_to_stompbox.find(instance_id);
    if (stompbox_instance == _instances_id_to_stompbox.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    auto parameter = stompbox_instance->second->get_parameter(param_id);
    if ( !parameter)
    {
        return EngineReturnStatus::INVALID_PARAMETER_UID;
    }
    switch(parameter->type())
    {
        case StompBoxParameterType::FLOAT:
        {
            static_cast<FloatStompBoxParameter*>(parameter)->set(value);
            break;
        }
        case StompBoxParameterType::INT:
        {
            static_cast<IntStompBoxParameter*>(parameter)->set(static_cast<int>(value));
            break;
        }
        case StompBoxParameterType::BOOL:
        {
            static_cast<IntStompBoxParameter*>(parameter)->set(value > 0.5f ? true : false);
            break;
        }
        default: {}
    }
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::send_stompbox_event(const std::string& instance_id, BaseEvent* event)
{
    auto stompbox_instance = _instances_id_to_stompbox.find(instance_id);
    if (stompbox_instance == _instances_id_to_stompbox.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    stompbox_instance->second->instance()->process_event(event);
    return EngineReturnStatus::OK;
}


} // namespace engine
} // namespace sushi