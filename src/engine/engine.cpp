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
                                                     const std::string& processor_id,
                                                     const std::string& parameter,
                                                     float min_range,
                                                     float max_range,
                                                     int midi_channel)
{
    if (midi_port >= _midi_inputs || midi_channel < 0 || midi_channel > midi::MidiChannel::OMNI)
    {
        return EngineReturnStatus::INVALID_ARGUMENTS;
    }
    auto processor_node = _processors_by_unique_name.find(processor_id);
    if (processor_node == _processors_by_unique_name.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    /* We have already checked that the processor and parameter exist, so we can assume this will succeed */
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
    auto processor_node = _processors_by_unique_name.find(chain_id);
    if (processor_node == _processors_by_unique_name.end())
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    /* We have already checked that the processor exist, so we can assume this will succeed */
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

std::unique_ptr<Processor> AudioEngine::_make_stompbox_from_unique_id(const std::string& uid)
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

EngineReturnStatus AudioEngine::_fill_chain_from_json_definition(const Json::Value& chain_def)
{
    PluginChain* chain = new PluginChain;
    EngineReturnStatus status = set_up_channel_config(*chain, chain_def["mode"]);
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
            instance->init(_sample_rate);
            if (instance == nullptr)
            {
                MIND_LOG_ERROR("Invalid plugin uid {} in configuration file for chain {}", uid, chain_def["id"].asString());
                return EngineReturnStatus::INVALID_STOMPBOX_UID;
            }
            // TODO - at some point test if the name actually is unique.
            auto name = stompbox_def["id"].asString();
            instance->set_name(name);
            chain->add(instance.get());
            _register_processor(std::move(instance), name);
        }
        _audio_graph.push_back(chain);
        _register_processor(std::move(std::unique_ptr<Processor>(chain)), chain_def["id"].asString());
    }
    else
    {
        MIND_LOG_ERROR("Invalid format for stompbox chain n. {} in configuration file", chain_def["id"].asString());
        delete chain;
        return EngineReturnStatus::INVALID_STOMPBOX_CHAIN;
    }
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::_register_processor(std::unique_ptr<Processor> processor, const std::string& str_id)
{
    auto existing = _processors_by_unique_name.find(str_id);
    if (existing != _processors_by_unique_name.end())
    {
        /*---------- TODO: If processor name exists, this deletes the processor*.
        A better way is needed here. ----------*/
        MIND_LOG_WARNING("Processor with this name already exists");
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    if (processor->id() > _processors_by_unique_id.size())
    {
        // Resize the vector manually to be able to insert processors at specific indexes
        _processors_by_unique_id.resize(_processors_by_unique_id.size() + PROC_ID_ARRAY_INCREMENT, nullptr);
    }
    processor->set_name(str_id);
    _processors_by_unique_id[processor->id()] = processor.get();
    _processors_by_unique_name[str_id] = std::move(processor);
    return EngineReturnStatus::OK;
}

bool AudioEngine::_processor_exists(const std::string& name)
{
    auto processor_node = _processors_by_unique_name.find(name);
    if(processor_node == _processors_by_unique_name.end())
    {
        return false;
    }
    return true;
}

bool AudioEngine::_processor_exists(ObjectId uid)
{
    if(uid >= _processors_by_unique_id.size() || !_processors_by_unique_id[uid])
    {
        return false;
    }
    return true;
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

EngineReturnStatus AudioEngine::send_rt_event(Event event)
{
    if (event.processor_id() > _processors_by_unique_id.size())
    {
        MIND_LOG_WARNING("Invalid stompbox id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    auto processor_node = _processors_by_unique_id[event.processor_id()];
    if (!processor_node)
    {
        MIND_LOG_WARNING("Invalid stompbox id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    processor_node->process_event(event);
    return EngineReturnStatus::OK;
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::processor_id_from_name(const std::string& name)
{
    auto processor_node = _processors_by_unique_name.find(name);
    if (processor_node == _processors_by_unique_name.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_STOMPBOX_UID, 0);
    }
    return std::make_pair(EngineReturnStatus::OK, processor_node->second->id());
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::parameter_id_from_name(const std::string& processor_name,
                                                                            const std::string& parameter_name)
{
    auto processor_node = _processors_by_unique_name.find(processor_name);
    if (processor_node == _processors_by_unique_name.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_STOMPBOX_UID, 0);
    }
    ProcessorReturnCode status;
    ObjectId id;
    std::tie(status, id) = processor_node->second->parameter_id_from_name(parameter_name);
    if (status != ProcessorReturnCode::OK)
    {
        return std::make_pair(EngineReturnStatus::INVALID_PARAMETER_UID, 0);
    }
    return std::make_pair(EngineReturnStatus::OK, id);
};

std::pair<EngineReturnStatus, const std::string> AudioEngine::processor_name_from_id(ObjectId uid)
{
    if (uid >= _processors_by_unique_id.size() || !_processors_by_unique_id[uid])
    {
        return std::make_pair(EngineReturnStatus::INVALID_STOMPBOX_UID, std::string(""));
    }
    return std::make_pair(EngineReturnStatus::OK, _processors_by_unique_id[uid]->name());
}

EngineReturnStatus AudioEngine::create_plugin_chain(const std::string& chain_name, int chain_channel_count)
{
    if(chain_name.empty())
    {
        MIND_LOG_ERROR("Chain name is not specified");
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    if((chain_channel_count != 1 && chain_channel_count != 2))
    {
        MIND_LOG_ERROR("Invalid number of channels");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    if(_processor_exists(chain_name))
    {
        MIND_LOG_ERROR("Chain name already exists in processor list{} ", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }

    PluginChain* chain = new PluginChain;
    chain->set_input_channels(chain_channel_count);
    chain->set_output_channels(chain_channel_count);
    EngineReturnStatus status = _register_processor(std::move(std::unique_ptr<Processor>(chain)), chain_name);
    _audio_graph.push_back(chain);
    return status;
}

EngineReturnStatus AudioEngine::add_plugin_to_chain(const std::string& chain_name,
                                                    const std::string& plugin_uid,
                                                    const std::string& plugin_name)
{
    EngineReturnStatus status;
    if(chain_name.empty())
    {
        MIND_LOG_ERROR("Chain name is not specified");
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    if(plugin_uid.empty())
    {
        MIND_LOG_ERROR("Plugin UID is not specified");
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }
    if(plugin_name.empty())
    {
        MIND_LOG_ERROR("Plugin name is not specified");
        return EngineReturnStatus::INVALID_STOMPBOX_NAME;
    }
    if(_processor_exists(plugin_name))
    {
        MIND_LOG_ERROR("Plugin name {} already exists", plugin_name);
        return EngineReturnStatus::INVALID_STOMPBOX_NAME;
    }
    auto instance = _make_stompbox_from_unique_id(plugin_uid);
    if(instance == nullptr)
    {
        MIND_LOG_ERROR("Invalid plugin uid {} ", plugin_uid);
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }

    /* Get chain ID and add plugin to chain */
    ObjectId chain_id;
    std::tie(status, chain_id) = processor_id_from_name(chain_name);
    if(status == EngineReturnStatus::INVALID_STOMPBOX_UID)
    {
        MIND_LOG_ERROR("Chain name {} does not exist", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }

    instance->init(_sample_rate);
    /**
        The following static cast assumes that the existing processor
        is a PluginChain*. This isnt safe.
        TODO:
        - Find a way to denote the processor type.
        Maybe in the base Processor class?
    */
    auto chain = static_cast<PluginChain*>(_processors_by_unique_id[chain_id]);
    chain->add(instance.get());
    status = _register_processor(std::move(instance), plugin_name);
    return status;
}

} // namespace engine
} // namespace sushi
