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

int get_midi_channel_from_json(const Json::Value& value)
{
    if (value.isString())
    {
        if (value.asString() == "omni" || value.asString() == "all" )
        {
            return midi::MidiChannel::OMNI;
        }
        return -1;
    }
    else
    {
        return value.asInt();
    }
}

AudioEngine::AudioEngine(int sample_rate) : BaseEngine::BaseEngine(sample_rate)
{}

AudioEngine::~AudioEngine()
{}


EngineReturnStatus AudioEngine::connect_midi_cc_data(int midi_port,
                                                     int cc_no,
                                                     const std::string &processor_id,
                                                     const std::string &parameter,
                                                     float min_range,
                                                     float max_range,
                                                     int midi_channel)
{
    if (midi_port >= _midi_inputs || midi_channel < 0 || midi_channel > midi::MidiChannel::OMNI)
    {
        return EngineReturnStatus::INVALID_ARGUMENTS;
    }
    auto processor_node = _instances_id_to_processors.find(processor_id);
    if (processor_node == _instances_id_to_processors.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    // Eventually get the set parameter and its range here
    _midi_dispatcher.connect_cc_to_parameter(midi_port, processor_id, parameter, cc_no, min_range, max_range, midi_channel);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_midi_kb_data(int midi_port,
                                        const std::string& chain_id,
                                        int midi_channel)
{
    if (midi_port >= _midi_inputs)
    {
        return EngineReturnStatus::INVALID_ARGUMENTS;
    }
    auto processor_node = _instances_id_to_processors.find(chain_id);
    if (processor_node == _instances_id_to_processors.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    _midi_dispatcher.connect_kb_to_track(midi_port, chain_id, midi_channel);
    return EngineReturnStatus::OK;
}


int AudioEngine::n_channels_in_chain(int chain)
{
    if (chain <= static_cast<int>(_audio_graph.size()))
    {
        return _audio_graph[chain]->input_channels();
    }
    return 0;
}

std::unique_ptr<Processor> AudioEngine::_make_stompbox_from_unique_id(const std::string &uid)
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

    return std::unique_ptr<Processor>(instance);
}

EngineReturnStatus AudioEngine::_fill_chain_from_json_definition(const Json::Value &chain_def)
{
    PluginChain* chain = new PluginChain;
    EngineReturnStatus status = set_up_channel_config(*chain, chain_def["mode"]);
    _audio_graph.push_back(chain);
    _instances_id_to_processors[chain_def["id"].asString()] = std::move(std::unique_ptr<Processor>(chain));
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
                MIND_LOG_ERROR("Invalid plugin uid {} in configuration file for chain {}", uid, chain_def["id"].asString());
                return EngineReturnStatus::INVALID_STOMPBOX_UID;
            }

            auto instance_id = stompbox_def["id"].asString();
            _instances_id_to_processors[instance_id] = std::move(instance);
            // TODO - look over ownership here - see ardours use of shared_ptr for instance
            chain->add(_instances_id_to_processors[instance_id].get());
            _instances_id_to_processors[instance_id]->init(_sample_rate);
        }
    }
    else
    {
        MIND_LOG_ERROR("Invalid format for stompbox chain n. {} in configuration file", chain_def["id"].asString());
        return EngineReturnStatus::INVALID_STOMPBOX_CHAIN;
    }
    return EngineReturnStatus::OK;
}

// TODO: eventually when configuration complexity grows, move this stuff in a separate class
EngineReturnStatus AudioEngine::init_chains_from_json_array(const Json::Value &chains)
{
    if (!chains.isArray() || chains.size() == 0)
    {
        MIND_LOG_ERROR("Incorrect number of stompbox chains ({}) in configuration file", chains.size());
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    for (auto& chain : chains)
    {
        EngineReturnStatus ret_code = _fill_chain_from_json_definition(chain);
        if (ret_code != EngineReturnStatus::OK)
        {
            return ret_code;
        }
    }
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::init_midi_from_json_array(const Json::Value &midi)
{
    if (midi.empty())
    {
        MIND_LOG_WARNING("No midi connections.");
        return EngineReturnStatus::OK;
    }
    const Json::Value& chain_connections = midi["chain_connections"];
    for (const Json::Value& con : chain_connections)
    {
        auto res = this->connect_midi_kb_data(con["port"].asInt(),
                                              con["chain"].asString(),
                                              get_midi_channel_from_json(con["channel"]));
        if (res != EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Error {} in setting up midi connections to chain {}.", (int)res, con["chain"].asString());
            return EngineReturnStatus::INVALID_ARGUMENTS;
        }
    }
    const Json::Value& cc_mappings = midi["cc_mappings"];
    for (const Json::Value& mapping : cc_mappings)
    {
        auto res = this->connect_midi_cc_data(mapping["port"].asInt(),
                                              mapping["cc_number"].asInt(),
                                              mapping["processor"].asString(),
                                              mapping["parameter"].asString(),
                                              mapping["min_range"].asFloat(),
                                              mapping["max_range"].asFloat(),
                                              get_midi_channel_from_json(mapping["channel"]));
        if (res != EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Error {} in setting upp midi cc mappings for {}.", (int)res, mapping["processor"].asString());
            return EngineReturnStatus::INVALID_ARGUMENTS;
        }
    }
    return EngineReturnStatus::OK;
}


void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    /* Put the channels from in_buffer into the audio graph based on the graphs channel count
     * Note that its assumed that number of input and output channels are equal. */

    int start_channel = 0;
    for (auto& graph : _audio_graph)
    {
        int no_of_channels = graph->input_channels();
        if (start_channel + no_of_channels <= in_buffer->channel_count())
        {
            ChunkSampleBuffer ch_in = ChunkSampleBuffer::create_non_owning_buffer(*in_buffer,
                                                                                  start_channel,
                                                                                  no_of_channels);
            ChunkSampleBuffer ch_out = ChunkSampleBuffer::create_non_owning_buffer(*out_buffer,
                                                                                   start_channel,
                                                                                   no_of_channels);
            graph->process_audio(ch_in, ch_out);
            start_channel += no_of_channels;
        } else
        {
            break;
        }
    }
    if (start_channel < in_buffer->channel_count())
    {
        MIND_LOG_WARNING("Warning, not all input channels processed, {} out of {} processed",
                         start_channel,
                         in_buffer->channel_count());
    }
}


EngineReturnStatus AudioEngine::send_rt_event(BaseEvent* event)
{
    assert(event);
    auto processor_node = _instances_id_to_processors.find(event->processor_id());
    if (processor_node == _instances_id_to_processors.end())
    {
        MIND_LOG_WARNING("Invalid stompbox id {}.", event->processor_id());
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    processor_node->second->process_event(event);
    return EngineReturnStatus::OK;
}

} // namespace engine
} // namespace sushi