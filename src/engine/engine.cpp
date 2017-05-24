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
    if(!_processor_exists(processor_id))
    {
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }   _midi_dispatcher.connect_cc_to_parameter(midi_port, processor_id, parameter, cc_no, min_range, max_range, midi_channel);
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
    if(!_processor_exists(chain_id))
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
    EngineReturnStatus status;
    ObjectId chain_id;
    std::tie(status, chain_id) = processor_id_from_name(chain_name);
    if(status == EngineReturnStatus::INVALID_STOMPBOX_UID)
    {
        MIND_LOG_ERROR("Chain name {} does not exist in processor list", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    auto instance = _make_stompbox_from_unique_id(plugin_uid);
    if(instance == nullptr)
    {
        MIND_LOG_ERROR("Invalid plugin uid {} ", plugin_uid);
        return EngineReturnStatus::INVALID_STOMPBOX_UID;
    }

    instance->init(_sample_rate);

    /* TODO: Static cast isnt safe. Need mechanisim to denote processor type.*/
    auto chain = static_cast<PluginChain*>(_processors_by_unique_id[chain_id]);
    chain->add(instance.get());
    status = _register_processor(std::move(instance), plugin_name);
    return status;
}

} // namespace engine
} // namespace sushi