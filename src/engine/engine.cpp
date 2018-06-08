#include "twine/src/twine_internal.h"

#include "engine.h"
#include "logging.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/equalizer_plugin.h"
#include "plugins/arpeggiator_plugin.h"
#include "plugins/sample_player_plugin.h"
#include "plugins/peak_meter_plugin.h"
#include "library/vst2x_wrapper.h"
#include "library/vst3x_wrapper.h"

namespace sushi {
namespace engine {

constexpr auto RT_EVENT_TIMEOUT = std::chrono::milliseconds(200);

MIND_GET_LOGGER_WITH_MODULE_NAME("engine");

AudioEngine::AudioEngine(float sample_rate) : BaseEngine::BaseEngine(sample_rate),
                                              _transport(sample_rate)
{
    _event_dispatcher.run();
}

AudioEngine::~AudioEngine()
{
    _event_dispatcher.stop();
}

void AudioEngine::set_sample_rate(float sample_rate)
{
    BaseEngine::set_sample_rate(sample_rate);
    for (auto& node : _processors)
    {
        node.second->configure(sample_rate);
    }
    _transport.set_sample_rate(sample_rate);
}

EngineReturnStatus AudioEngine::connect_audio_input_channel(int input_channel, int track_channel, const std::string& track_name)
{
    auto processor_node = _processors.find(track_name);
    if(processor_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_TRACK;
    }
    auto track = static_cast<Track*>(processor_node->second.get());
    if (input_channel >= _audio_inputs || track_channel >= track->input_channels())
    {
        return EngineReturnStatus::INVALID_CHANNEL;
    }
    Connection con = {input_channel, track_channel, track->id()};
    _in_audio_connections.push_back(con);
    MIND_LOG_INFO("Connected inputs {} to channel {} of track \"{}\"", input_channel, track_channel, track_name);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_audio_output_channel(int output_channel, int track_channel,
                                                             const std::string& track_name)
{
    auto processor_node = _processors.find(track_name);
    if(processor_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_TRACK;
    }
    auto track = static_cast<Track*>(processor_node->second.get());
    if (output_channel >= _audio_outputs || track_channel >= track->output_channels())
    {
        return EngineReturnStatus::INVALID_CHANNEL;
    }
    Connection con = {output_channel, track_channel, track->id()};
    _out_audio_connections.push_back(con);
    MIND_LOG_INFO("Connected channel {} of track \"{}\" to output {}", track_channel, track_name, output_channel);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_audio_input_bus(int input_bus, int track_bus, const std::string& track_name)
{
    auto status = connect_audio_input_channel(input_bus * 2, track_bus * 2, track_name);
    if (status != EngineReturnStatus::OK)
    {
        return status;
    }
    return connect_audio_input_channel(input_bus * 2 + 1, track_bus * 2 + 1, track_name);
}

EngineReturnStatus AudioEngine::connect_audio_output_bus(int output_bus, int track_bus, const std::string& track_name)
{
    auto status = connect_audio_output_channel(output_bus * 2, track_bus * 2, track_name);
    if (status != EngineReturnStatus::OK)
    {
        return status;
    }
    return connect_audio_output_channel(output_bus * 2 + 1, track_bus * 2 + 1, track_name);
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
            auto event = RtEvent::make_stop_engine_event();
            send_async_event(event);
        } else
        {
            _state.store(RealtimeState::STOPPED);
        }
    }
};

int AudioEngine::n_channels_in_track(int track)
{
    if (track <= static_cast<int>(_audio_graph.size()))
    {
        return _audio_graph[track]->input_channels();
    }
    return 0;
}

Processor* AudioEngine::_make_internal_plugin(const std::string& uid)
{
    Processor* instance = nullptr;
    if (uid == "sushi.testing.passthrough")
    {
        instance = new passthrough_plugin::PassthroughPlugin(_host_control);
    }
    else if (uid == "sushi.testing.gain")
    {
        instance = new gain_plugin::GainPlugin(_host_control);
    }
    else if (uid == "sushi.testing.equalizer")
    {
        instance = new equalizer_plugin::EqualizerPlugin(_host_control);
    }
    else if (uid == "sushi.testing.sampleplayer")
    {
        instance = new sample_player_plugin::SamplePlayerPlugin(_host_control);
    }
    else if (uid == "sushi.testing.arpeggiator")
    {
        instance = new arpeggiator_plugin::ArpeggiatorPlugin(_host_control);
    }
    else if (uid == "sushi.testing.arpeggiator")
    {
        instance = new arpeggiator_plugin::ArpeggiatorPlugin(_host_control);
    }
    else if (uid == "sushi.testing.peakmeter")
    {
        instance = new peak_meter_plugin::PeakMeterPlugin(_host_control);
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
        /* TODO - When non-rt callbacks for events are ready we can have the
         * rt processor list re-allocated outside the rt domain
         * In the meantime, put alilmit in the number of processors */
        MIND_LOG_ERROR("Realtime processor list full");
        assert(false);
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
    /* Signal that this is a realtime audio processing thread */
    twine::ThreadRtFlag rt_flag;

    RtEvent in_event;
    while (_internal_control_queue.pop(in_event))
    {
        send_rt_event(in_event);
    }
    while (_main_in_queue.pop(in_event))
    {
        send_rt_event(in_event);
    }

    _event_dispatcher.set_time(_transport.current_process_time());
    auto state = _state.load();

    for (const auto& c : _in_audio_connections)
    {
        auto engine_in = ChunkSampleBuffer::create_non_owning_buffer(*in_buffer, c.engine_channel, 1);
        auto track_in = static_cast<Track*>(_realtime_processors[c.track])->input_channel(c.track_channel);
        track_in = engine_in;
    }

    for (auto& track : _audio_graph)
    {
        track->render();
    }

    _main_out_queue.push(RtEvent::make_synchronisation_event(_transport.current_process_time()));

    out_buffer->clear();
    for (const auto& c : _out_audio_connections)
    {
        auto track_out = static_cast<Track*>(_realtime_processors[c.track])->output_channel(c.track_channel);
        auto engine_out = ChunkSampleBuffer::create_non_owning_buffer(*out_buffer, c.engine_channel, 1);
        engine_out.add(track_out);
    }

    _state.store(update_state(state));
}

void AudioEngine::set_tempo(float tempo)
{
    if (_state.load() == RealtimeState::STOPPED)
    {
        _transport.set_tempo(tempo);
    }
    else
    {
        auto e = RtEvent::make_tempo_event(0, tempo);
        send_async_event(e);
    }
}

void AudioEngine::set_time_signature(TimeSignature signature)
{
    if (_state.load() == RealtimeState::STOPPED)
    {
        _transport.set_time_signature(signature);
    }
    else
    {
        auto e = RtEvent::make_time_signature_event(0, signature);
        send_async_event(e);
    }
}

void AudioEngine::set_transport_mode(PlayingMode mode)
{
    if (_state.load() == RealtimeState::STOPPED)
    {
        _transport.set_playing_mode(mode);
    }
    else
    {
        auto e = RtEvent::make_playing_mode_event(0, mode);
        send_async_event(e);
    }
}

void AudioEngine::set_tempo_sync_mode(SyncMode mode)
{
    if (_state.load() == RealtimeState::STOPPED)
    {
        _transport.set_sync_mode(mode);
    }
    else
    {
        auto e = RtEvent::make_sync_mode_event(0, mode);
        send_async_event(e);
    }
}

EngineReturnStatus AudioEngine::send_rt_event(RtEvent& event)
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

EngineReturnStatus AudioEngine::send_async_event(RtEvent& event)
{
    std::lock_guard<std::mutex> lock(_in_queue_lock);
    if (_internal_control_queue.push(event))
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

std::pair<EngineReturnStatus, const std::string> AudioEngine::parameter_name_from_id(const std::string &processor_name,
                                                                                     const ObjectId id)
{
    auto processor_node = _processors.find(processor_name);
    if (processor_node == _processors.end())
    {
        return std::make_pair(EngineReturnStatus::INVALID_PROCESSOR, "");
    }
    auto param = processor_node->second->parameter_from_id(id);
    if (param)
    {
        return std::make_pair(EngineReturnStatus::OK, param->name());
    }
    return std::make_pair(EngineReturnStatus::INVALID_PARAMETER, "");
}

EngineReturnStatus AudioEngine::create_multibus_track(const std::string& name, int input_busses, int output_busses)
{
    if((input_busses > TRACK_MAX_BUSSES && output_busses > TRACK_MAX_BUSSES))
    {
        MIND_LOG_ERROR("Invalid number of busses for new track");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    Track* track = new Track(_host_control, input_busses, output_busses);
    return _register_new_track(name, track);
}

EngineReturnStatus AudioEngine::create_track(const std::string &name, int channel_count)
{
    if((channel_count != 1 && channel_count != 2))
    {
        MIND_LOG_ERROR("Invalid number of channels for new track");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    Track* track = new Track(_host_control, channel_count);
    return _register_new_track(name, track);
}

EngineReturnStatus AudioEngine::delete_track(const std::string &track_name)
{
    // TODO - Until it's decided how tracks report what processors they have,
    // we assume that the track has no processors before deleting
    auto track_node = _processors.find(track_name);
    if (track_node == _processors.end())
    {
        MIND_LOG_ERROR("Couldn't delete track {}, not found", track_name);
        return EngineReturnStatus::INVALID_TRACK;
    }
    auto track = track_node->second.get();
    if (realtime())
    {
        auto remove_track_event = RtEvent::make_remove_track_event(track->id());
        auto delete_event = RtEvent::make_remove_processor_event(track->id());
        send_async_event(remove_track_event);
        send_async_event(delete_event);
        bool removed = _event_receiver.wait_for_response(remove_track_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool deleted = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!removed || !deleted)
        {
            MIND_LOG_ERROR("Failed to remove processor {} from processing part", track_name);
        }
        return _deregister_processor(track_name);
    }
    else
    {
        for (auto track_in_graph = _audio_graph.begin(); track_in_graph != _audio_graph.end(); ++track)
        {
            if (*track_in_graph == track)
            {
                _audio_graph.erase(track_in_graph);
                _remove_processor_from_realtime_part(track->id());
                return _deregister_processor(track_name);
            }
            MIND_LOG_WARNING("Plugin track {} was not in the audio graph", track_name);
        }
        return EngineReturnStatus::INVALID_TRACK;
    }
}

EngineReturnStatus AudioEngine::add_plugin_to_track(const std::string &track_name,
                                                    const std::string &plugin_uid,
                                                    const std::string &plugin_name,
                                                    const std::string &plugin_path,
                                                    PluginType plugin_type)
{
    auto track_node = _processors.find(track_name);
    if (track_node == _processors.end())
    {
        MIND_LOG_ERROR("Track named {} does not exist in processor list", track_name);
        return EngineReturnStatus::INVALID_TRACK;
    }
    auto track = static_cast<Track*>(track_node->second.get());
    Processor* plugin{nullptr};
    switch (plugin_type)
    {
        case PluginType::INTERNAL:
            plugin = _make_internal_plugin(plugin_uid);
            if(plugin == nullptr)
            {
                MIND_LOG_ERROR("Unrecognised internal plugin \"{}\"", plugin_uid);
                return EngineReturnStatus::INVALID_PLUGIN_UID;
            }
            break;

        case PluginType::VST2X:
            plugin = new vst2::Vst2xWrapper(_host_control, plugin_path, &_event_dispatcher);
            break;

        case PluginType::VST3X:
            plugin = new vst3::Vst3xWrapper(_host_control, plugin_path, plugin_uid);
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
        auto insert_event = RtEvent::make_insert_processor_event(plugin);
        auto add_event = RtEvent::make_add_processor_to_track_event(plugin->id(), track->id());
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
        if (track->add(plugin) == false)
        {
            return EngineReturnStatus::ERROR;
        }
    }
    return EngineReturnStatus::OK;
}

/* TODO - In the future it should be possible to remove plugins without deleting them
 * and consequentally to add them to a different track or have plugins not associated
 * to a particular track. */
EngineReturnStatus AudioEngine::remove_plugin_from_track(const std::string &track_name, const std::string &plugin_name)
{
    auto track_node = _processors.find(track_name);
    if (track_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_TRACK;
    }
    auto processor_node = _processors.find(plugin_name);
    if (processor_node == _processors.end())
    {
        return EngineReturnStatus::INVALID_PLUGIN_NAME;
    }
    auto processor = processor_node->second.get();
    Track* track = static_cast<Track*>(track_node->second.get());
    if (realtime())
    {
        // Send events to handle this in the rt domain
        auto remove_event = RtEvent::make_remove_processor_from_track_event(processor->id(), track->id());
        auto delete_event = RtEvent::make_remove_processor_event(processor->id());
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
        if (!track->remove(processor->id()))
        {
            MIND_LOG_ERROR("Failed to remove processor {} from track {}", plugin_name, track_name);
        }
        _remove_processor_from_realtime_part(processor->id());
    }
    return _deregister_processor(processor->name());
}

EngineReturnStatus AudioEngine::_register_new_track(const std::string& name, Track* track)
{
    auto status = _register_processor(track, name);
    if (status != EngineReturnStatus::OK)
    {
        delete track;
        return status;
    }
    track->set_event_output(&_main_out_queue);
    if (realtime())
    {
        auto insert_event = RtEvent::make_insert_processor_event(track);
        auto add_event = RtEvent::make_add_track_event(track->id());
        send_async_event(insert_event);
        send_async_event(add_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool added = _event_receiver.wait_for_response(add_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted || !added)
        {
            MIND_LOG_ERROR("Failed to insert/add track {} to processing part", name);
            return EngineReturnStatus::INVALID_PROCESSOR;
        }
    } else
    {
        _insert_processor_in_realtime_part(track);
        _audio_graph.push_back(track);
    }
    MIND_LOG_INFO("Track {} successfully added to engine", name);
    return EngineReturnStatus::OK;
}

bool AudioEngine::_handle_internal_events(RtEvent &event)
{
    switch (event.type())
    {
        case RtEventType::STOP_ENGINE:
        {
            auto typed_event = event.returnable_event();
            _state.store(RealtimeState::STOPPING);
            typed_event->set_handled(true);
            break;
        }
        case RtEventType::INSERT_PROCESSOR:
        {
            auto typed_event = event.processor_operation_event();
            bool ok = _insert_processor_in_realtime_part(typed_event->instance());
            typed_event->set_handled(ok);
            break;
        }
        case RtEventType::REMOVE_PROCESSOR:
        {
            auto typed_event = event.processor_reorder_event();
            bool ok = _remove_processor_from_realtime_part(typed_event->processor());
            typed_event->set_handled(ok);
            break;
        }
        case RtEventType::ADD_PROCESSOR_TO_TRACK:
        {
            auto typed_event = event.processor_reorder_event();
            Track* track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
            Processor* processor = static_cast<Processor*>(_realtime_processors[typed_event->processor()]);
            if (track && processor)
            {
                auto ok = track->add(processor);
                typed_event->set_handled(ok);
            }
            else
            {
                typed_event->set_handled(false);
            }
            break;
        }
        case RtEventType::REMOVE_PROCESSOR_FROM_TRACK:
        {
            auto typed_event = event.processor_reorder_event();
            Track* track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
            if (track)
            {
                bool ok = track->remove(typed_event->processor());
                typed_event->set_handled(ok);
            }
            else
                typed_event->set_handled(true);
            break;
        }
        case RtEventType::ADD_TRACK:
        {
            auto typed_event = event.processor_reorder_event();
            Track* track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
            if (track)
            {
                _audio_graph.push_back(track);
                typed_event->set_handled(true);
            }
            else
                typed_event->set_handled(false);
            break;
        }
        case RtEventType::REMOVE_TRACK:
        {
            auto typed_event = event.processor_reorder_event();
            Track* track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
            if (track)
            {
                for (auto i = _audio_graph.begin(); i != _audio_graph.end(); ++i)
                {
                    if ((*i)->id() == typed_event->track())
                    {
                        _audio_graph.erase(i);
                        typed_event->set_handled(true);
                        break;
                    }
                }
            }
            else
                typed_event->set_handled(false);
            break;
        }
        case RtEventType::SET_BYPASS:
        {
            auto typed_event = event.processor_command_event();
            auto processor = static_cast<Track*>(_realtime_processors[typed_event->processor_id()]);
            if (processor)
            {
                processor->set_bypassed(typed_event->value());
            }
            return true;
        }
        case RtEventType::TEMPO:
        {
            /* Eventually we might want to do sample accurate tempo changes */
            _transport.set_tempo(event.tempo_event()->tempo());
            break;
        }

        case RtEventType::TIME_SIGNATURE:
        {
            /* Eventually we might want to do sample accurate time signature changes */
            _transport.set_time_signature(event.time_signature_event()->time_signature());
            break;
        }

        case RtEventType::PLAYING_MODE:
        {
            _transport.set_playing_mode(event.playing_mode_event()->mode());
            break;
        }

        case RtEventType::SYNC_MODE:
        {
            _transport.set_sync_mode(event.sync_mode_event()->mode());
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
