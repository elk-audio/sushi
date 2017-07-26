#include "engine.h"

#include <iostream>

#include "logging.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"
#include "plugins/sample_player_plugin.h"
#include "library/vst2x_wrapper.h"
#include "library/vst3x_wrapper.h"


namespace sushi {
namespace engine {

MIND_GET_LOGGER;

AudioEngine::AudioEngine(int sample_rate) : BaseEngine::BaseEngine(sample_rate)
{}

AudioEngine::~AudioEngine()
{}

int AudioEngine::n_channels_in_chain(int chain)
{
    if (chain <= static_cast<int>(_audio_graph.size()))
    {
        return _audio_graph[chain]->input_channels();
    }
    return 0;
}

std::unique_ptr<Processor> AudioEngine::_make_internal_plugin(const std::string& uid)
{
    Processor* instance = nullptr;
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
    if(_processor_exists(str_id))
    {
        MIND_LOG_WARNING("Processor with this name already exists");
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    if (processor->id() > _processors_by_unique_id.size())
    {
        // Resize the vector manually to be able to insert processors at specific indexes
        _processors_by_unique_id.resize(_processors_by_unique_id.size() + PROC_ID_ARRAY_INCREMENT, nullptr);
    }
    processor->set_name(str_id);
    _processors_by_unique_id[processor->id()] = processor.get();
    _processors_by_unique_name[str_id] = std::move(processor);
    MIND_LOG_DEBUG("Succesfully registered processor {}.", str_id);
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

bool AudioEngine::_processor_exists(const ObjectId uid)
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
        MIND_LOG_WARNING("Invalid processor id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    auto processor_node = _processors_by_unique_id[event.processor_id()];
    if (!processor_node)
    {
        MIND_LOG_WARNING("Invalid processor id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    processor_node->process_event(event);
    return EngineReturnStatus::OK;
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::processor_id_from_name(const std::string& name)
{
    auto processor_node = _processors_by_unique_name.find(name);
    if (processor_node == _processors_by_unique_name.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_PROCESSOR, 0);
    }
    return std::make_pair(EngineReturnStatus::OK, processor_node->second->id());
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::parameter_id_from_name(const std::string& processor_name,
                                                                            const std::string& parameter_name)
{
    auto processor_node = _processors_by_unique_name.find(processor_name);
    if (processor_node == _processors_by_unique_name.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_PROCESSOR, 0);
    }
    auto param = processor_node->second->parameter_from_name(parameter_name);
    if (param)
    {
        return std::make_pair(EngineReturnStatus::OK, param->id());
    }
    return std::make_pair(EngineReturnStatus::INVALID_PARAMETER, 0);
};

std::pair<EngineReturnStatus, const std::string> AudioEngine::processor_name_from_id(const ObjectId uid)
{
    if (!_processor_exists(uid))
    {
        return std::make_pair(EngineReturnStatus::INVALID_PROCESSOR, std::string(""));
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
    MIND_LOG_INFO("Plugin Chain {} successfully added to engine", chain_name);
    return status;
}

EngineReturnStatus AudioEngine::add_plugin_to_chain(const std::string& chain_name,
                                                    const std::string& plugin_uid,
                                                    const std::string& plugin_name,
                                                    const std::string& plugin_path,
                                                    PluginType plugin_type)
{
    if(plugin_name.empty())
    {
        MIND_LOG_ERROR("Plugin name is not specified");
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    if(_processor_exists(plugin_name))
    {
        MIND_LOG_ERROR("Plugin name {} already exists", plugin_name);
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    EngineReturnStatus status;
    ObjectId chain_id;
    std::tie(status, chain_id) = processor_id_from_name(chain_name);
    if(status == EngineReturnStatus::INVALID_PROCESSOR)
    {
        MIND_LOG_ERROR("Chain name {} does not exist in processor list", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }

    std::unique_ptr<Processor> plugin;
    switch (plugin_type)
    {
        case PluginType::INTERNAL:
            plugin = std::move(_make_internal_plugin(plugin_uid));
            if(plugin == nullptr)
            {
                MIND_LOG_ERROR("Incorrect stompbox UID \"{}\"", plugin_uid);
                return EngineReturnStatus::INVALID_PLUGIN_UID;
            }
            break;

        case PluginType::VST2X:
            plugin = std::make_unique<vst2::Vst2xWrapper>(plugin_path);
            break;

        case PluginType::VST3X:
            plugin = std::make_unique<vst3::Vst3xWrapper>(plugin_path, plugin_uid);
            break;

        default:
            MIND_LOG_ERROR("Plugin {} has invalid or unsupported plugin type", plugin_uid);
            return EngineReturnStatus::INVALID_PLUGIN_TYPE;

    }

    auto processor_status = plugin->init(_sample_rate);
    if(processor_status != ProcessorReturnCode::OK)
    {
        MIND_LOG_ERROR("Failed to initialize plugin {}", plugin_name);
        return EngineReturnStatus::INVALID_PLUGIN_UID;
    }
    plugin->set_enabled(true);
    auto chain = static_cast<PluginChain*>(_processors_by_unique_id[chain_id]);
    chain->add(plugin.get());
    status = _register_processor(std::move(plugin), plugin_name);
    return status;
}

} // namespace engine
} // namespace sushi