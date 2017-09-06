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

constexpr auto RT_EVENT_TIMEOUT = std::chrono::milliseconds(200);

MIND_GET_LOGGER;

AudioEngine::AudioEngine(float sample_rate) : BaseEngine::BaseEngine(sample_rate)
{}

AudioEngine::~AudioEngine()
{}

void AudioEngine::set_sample_rate(float sample_rate)
{
    BaseEngine::set_sample_rate(sample_rate);
    for (auto& node : _processors)
    {
        node.second->configure(sample_rate);
    }
}

bool AudioEngine::realtime()
{
    return _state.load() != RealtimeState::STOPPED;
}

void AudioEngine::enable_realtime(bool enabled)
{
    if (enabled)
    {
        _state.store(RealtimeState::STARTING);
    }
    else
    {
        if (realtime())
        {
            send_async_event(Event::make_stop_engine_event());
        } else
        {
            _state.store(RealtimeState::STOPPED);
        }
    }
};

int AudioEngine::n_channels_in_chain(int chain)
{
    if (chain <= static_cast<int>(_audio_graph.size()))
    {
        return _audio_graph[chain]->input_channels();
    }
    return 0;
}

Processor* AudioEngine::_make_internal_plugin(const std::string& uid)
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
    return instance;
}

EngineReturnStatus AudioEngine::_register_processor(Processor* processor, const std::string& name)
{
    if(name.empty())
    {
        MIND_LOG_ERROR("Plugin name is not specified");
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    if(_processor_exists(name))
    {
        MIND_LOG_WARNING("Processor with this name already exists");
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    processor->set_name(name);
    _processors[name] = std::move(std::unique_ptr<Processor>(processor));
    MIND_LOG_DEBUG("Succesfully registered processor {}.", name);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::_deregister_processor(const std::string &name)
{
    auto processor_node = _processors.find(name);
    if (processor_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    _processors.erase(processor_node);
    return EngineReturnStatus::OK;
}

bool AudioEngine::_processor_exists(const std::string& name)
{
    auto processor_node = _processors.find(name);
    if(processor_node == _processors.end())
    {
        return false;
    }
    return true;
}

bool AudioEngine::_processor_exists(const ObjectId uid)
{
    if(uid >= _realtime_processors.size() || _realtime_processors[uid] == nullptr)
    {
        return false;
    }
    return true;
}

bool AudioEngine::_insert_processor_in_realtime_part(Processor* processor)
{
    if (processor->id() > _realtime_processors.size())
    {
        // Resize the vector manually to be able to insert processors at specific indexes
        _realtime_processors.resize(processor->id() + PROC_ID_ARRAY_INCREMENT, nullptr);
    }
    if(_realtime_processors[processor->id()] != nullptr)
    {
        return false;
    }
    _realtime_processors[processor->id()] = processor;
    return true;
}

bool AudioEngine::_remove_processor_from_realtime_part(ObjectId processor)
{
    if(_realtime_processors[processor] == nullptr)
    {
        return false;
    }
    _realtime_processors[processor] = nullptr;
    return true;
}

void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer)
{
    Event in_event;
    while (_control_queue_in.pop(in_event))
    {
        send_rt_event(in_event);
    }
    /* Put the channels from in_buffer into the audio graph based on the graphs channel count
     * Note that its assumed that number of input and output channels are equal. */
    auto state = _state.load();

    int start_channel = 0;
    for (auto &graph : _audio_graph)
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
    MIND_LOG_WARNING_IF(start_channel < in_buffer->channel_count(),
                        "Warning, not all input channels processed, {} out of {} processed",
                        start_channel, in_buffer->channel_count());
    _state.store(update_state(state));
}

EngineReturnStatus AudioEngine::send_rt_event(Event event)
{
    if (_handle_internal_events(event))
    {
        return EngineReturnStatus::OK;
    }
    if (event.processor_id() > _realtime_processors.size())
    {
        MIND_LOG_WARNING("Invalid processor id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    auto processor_node = _realtime_processors[event.processor_id()];
    if (processor_node == nullptr)
    {
        MIND_LOG_WARNING("Invalid processor id {}.", event.processor_id());
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    processor_node->process_event(event);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::send_async_event(const Event &event)
{
    if (_control_queue_in.push(event))
    {
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::QUEUE_FULL;
}


std::pair<EngineReturnStatus, ObjectId> AudioEngine::processor_id_from_name(const std::string& name)
{
    auto processor_node = _processors.find(name);
    if (processor_node == _processors.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_PROCESSOR, 0);
    }
    return std::make_pair(EngineReturnStatus::OK, processor_node->second->id());
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::parameter_id_from_name(const std::string& processor_name,
                                                                            const std::string& parameter_name)
{
    auto processor_node = _processors.find(processor_name);
    if (processor_node == _processors.end())
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
    return std::make_pair(EngineReturnStatus::OK, _realtime_processors[uid]->name());
}

EngineReturnStatus AudioEngine::create_plugin_chain(const std::string& chain_name, int chain_channel_count)
{
    if((chain_channel_count != 1 && chain_channel_count != 2))
    {
        MIND_LOG_ERROR("Invalid number of channels");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    PluginChain* chain = new PluginChain;
    chain->set_input_channels(chain_channel_count);
    chain->set_output_channels(chain_channel_count);
    EngineReturnStatus status = _register_processor(chain, chain_name);
    if (status != EngineReturnStatus::OK)
    {
        delete chain;
        return status;
    }
    if (realtime())
    {
        auto insert_event = Event::make_insert_processor_event(chain);
        auto add_event = Event::make_add_plugin_chain_event(chain->id());
        send_async_event(insert_event);
        send_async_event(add_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool added = _event_receiver.wait_for_response(add_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted || !added)
        {
            MIND_LOG_ERROR("Failed to insert/add chain {} to processing part", chain_name);
            return EngineReturnStatus::INVALID_PROCESSOR;
        }
    } else
    {
        _insert_processor_in_realtime_part(chain);
        _audio_graph.push_back(chain);
    }
    MIND_LOG_INFO("Plugin Chain {} successfully added to engine", chain_name);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::delete_plugin_chain(const std::string &chain_name)
{
    // TODO - Until it's decided how chain report what processors they have,
    // we assume that the chain is manually emptied before deleting
    auto chain_node = _processors.find(chain_name);
    if (chain_node == _processors.end())
    {
        MIND_LOG_ERROR("Couldn't delete chain {}, not found", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    auto chain = chain_node->second.get();
    if (realtime())
    {
        auto remove_chain_event = Event::make_remove_plugin_chain_event(chain->id());
        auto delete_event = Event::make_remove_processor_event(chain->id());
        send_async_event(remove_chain_event);
        send_async_event(delete_event);
        bool removed = _event_receiver.wait_for_response(remove_chain_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool deleted = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!removed || !deleted)
        {
            MIND_LOG_ERROR("Failed to remove processor {} from processing part", chain_name);
        }
        return _deregister_processor(chain_name);
    }
    else
    {
        for (auto chain_in_graph = _audio_graph.begin(); chain_in_graph != _audio_graph.end(); ++chain)
        {
            if (*chain_in_graph == chain)
            {
                _audio_graph.erase(chain_in_graph);
                _remove_processor_from_realtime_part(chain->id());
                return _deregister_processor(chain_name);
            }
            MIND_LOG_WARNING("Plugin chain {} was not in the audio graph", chain_name);
        }
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
}

EngineReturnStatus AudioEngine::add_plugin_to_chain(const std::string& chain_name,
                                                    const std::string& plugin_uid,
                                                    const std::string& plugin_name,
                                                    const std::string& plugin_path,
                                                    PluginType plugin_type)
{
    auto chain_node = _processors.find(chain_name);
    if (chain_node == _processors.end())
    {
        MIND_LOG_ERROR("Chain name {} does not exist in processor list", chain_name);
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    auto chain = static_cast<PluginChain*>(chain_node->second.get());
    Processor* plugin;
    switch (plugin_type)
    {
        case PluginType::INTERNAL:
            plugin = _make_internal_plugin(plugin_uid);
            if(plugin == nullptr)
            {
                MIND_LOG_ERROR("Incorrect stompbox UID \"{}\"", plugin_uid);
                return EngineReturnStatus::INVALID_PLUGIN_UID;
            }
            break;

        case PluginType::VST2X:
            plugin = new vst2::Vst2xWrapper(plugin_path);
            break;

        case PluginType::VST3X:
            plugin = new vst3::Vst3xWrapper(plugin_path, plugin_uid);
            break;
    }

    auto processor_status = plugin->init(_sample_rate);
    if(processor_status != ProcessorReturnCode::OK)
    {
        MIND_LOG_ERROR("Failed to initialize plugin {}", plugin_name);
        return EngineReturnStatus::INVALID_PLUGIN_UID;
    }
    EngineReturnStatus status = _register_processor(plugin, plugin_name);
    if(status != EngineReturnStatus::OK)
    {
        MIND_LOG_ERROR("Failed to register plugin {}", plugin_name);
        delete plugin;
        return status;
    }
    plugin->set_enabled(true);
    if (realtime())
    {
        // In realtime mode we need to handle this in the audio thread
        auto insert_event = Event::make_insert_processor_event(plugin);
        auto add_event = Event::make_add_processor_to_chain_event(plugin->id(), chain->id());
        send_async_event(insert_event);
        send_async_event(add_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool added = _event_receiver.wait_for_response(add_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted || !added)
        {
            MIND_LOG_ERROR("Failed to insert/add processor {} to processing part", plugin_name);
            return EngineReturnStatus::INVALID_PROCESSOR;
        }
    }
    else
    {
        // If the engine is not running in realtime mode we can add the processor directly
        _insert_processor_in_realtime_part(plugin);
        chain->add(plugin);
    }
    return EngineReturnStatus::OK;
}

/* TODO - In the future it should be possible to remove plugins without deleting them
 * and consequentally to add them to a different track or have plugins not associated
 * to a particular track. */
EngineReturnStatus AudioEngine::remove_plugin_from_chain(const std::string &chain_name, const std::string &plugin_name)
{
    auto chain_node = _processors.find(chain_name);
    if (chain_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_PLUGIN_CHAIN;
    }
    auto processor_node = _processors.find(plugin_name);
    if (processor_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    auto processor = processor_node->second.get();
    PluginChain* chain = static_cast<PluginChain*>(chain_node->second.get());
    if (realtime())
    {
        // Send events to handle this in the rt domain
        auto remove_event = Event::make_remove_processor_from_chain_event(processor->id(), chain->id());
        auto delete_event = Event::make_remove_processor_event(processor->id());
        send_async_event(remove_event);
        send_async_event(delete_event);
        bool remove_ok = _event_receiver.wait_for_response(remove_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool delete_ok = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!remove_ok || !delete_ok)
        {
            MIND_LOG_ERROR("Failed to remove/delete processor {} from processing part", plugin_name);
        }
    }
    else
    {
        if (!chain->remove(processor->id()))
        {
            MIND_LOG_ERROR("Failed to remove processor {} from chain", plugin_name);
        }
        _remove_processor_from_realtime_part(processor->id());
    }
    return _deregister_processor(processor->name());
}

bool AudioEngine::_handle_internal_events(Event &event)
{
    switch (event.type())
    {
        case EventType::STOP_ENGINE:
        {
            auto typed_event = event.returnable_event();
            _state.store(RealtimeState::STOPPING);
            typed_event->set_handled(true);
            break;
        }
        case EventType::INSERT_PROCESSOR:
        {
            auto typed_event = event.processor_operation_event();
            bool ok = _insert_processor_in_realtime_part(typed_event->instance());
            typed_event->set_handled(ok);
            break;
        }
        case EventType::REMOVE_PROCESSOR:
        {
            auto typed_event = event.processor_reorder_event();
            bool ok = _remove_processor_from_realtime_part(typed_event->processor());
            typed_event->set_handled(ok);
            break;
        }
        case EventType::ADD_PROCESSOR_TO_CHAIN:
        {
            auto typed_event = event.processor_reorder_event();
            PluginChain* chain = static_cast<PluginChain*>(_realtime_processors[typed_event->chain()]);
            Processor* processor = static_cast<Processor*>(_realtime_processors[typed_event->processor()]);
            if (chain && processor)
            {
                chain->add(processor);
                typed_event->set_handled(true);
            }
            else
            {
                typed_event->set_handled(false);
            }
            break;
        }
        case EventType::REMOVE_PROCESSOR_FROM_CHAIN:
        {
            auto typed_event = event.processor_reorder_event();
            PluginChain* chain = static_cast<PluginChain*>(_realtime_processors[typed_event->chain()]);
            if (chain)
            {
                bool ok = chain->remove(typed_event->processor());
                typed_event->set_handled(ok);
            }
            else
                typed_event->set_handled(true);
            break;
        }
        case EventType::ADD_PLUGIN_CHAIN:
        {
            auto typed_event = event.processor_reorder_event();
            PluginChain* chain = static_cast<PluginChain*>(_realtime_processors[typed_event->chain()]);
            if (chain)
            {
                _audio_graph.push_back(chain);
                typed_event->set_handled(true);
            }
            else
                typed_event->set_handled(false);
            break;
        }
        case EventType::REMOVE_PLUGIN_CHAIN:
        {
            auto typed_event = event.processor_reorder_event();
            PluginChain* chain = static_cast<PluginChain*>(_realtime_processors[typed_event->chain()]);
            if (chain)
            {
                for (auto i = _audio_graph.begin(); i != _audio_graph.end(); ++i)
                {
                    if ((*i)->id() == typed_event->chain())
                    {
                        _audio_graph.erase(i);
                    }
                    typed_event->set_handled(true);
                    break;
                }
            }
            else
                typed_event->set_handled(false);
            break;
        }
        default:
            return false;
    }
    _control_queue_out.push(event); // Send event back to non-rt domain
    return true;
}

RealtimeState update_state(RealtimeState current_state)
{
    if (current_state == RealtimeState::STARTING)
    {
        return RealtimeState::RUNNING;
    }
    if (current_state == RealtimeState::STOPPING)
    {
        return RealtimeState::STOPPED;
    }
    return current_state;
}

} // namespace engine
} // namespace sushi