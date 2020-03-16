/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Real time audio processing engine
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <fstream>
#include <iomanip>
#include <functional>

#include "twine/src/twine_internal.h"

#include "audio_engine.h"
#include "logging.h"
#include "plugins/passthrough_plugin.h"
#include "plugins/gain_plugin.h"
#include "plugins/lfo_plugin.h"
#include "plugins/equalizer_plugin.h"
#include "plugins/arpeggiator_plugin.h"
#include "plugins/sample_player_plugin.h"
#include "plugins/peak_meter_plugin.h"
#include "plugins/transposer_plugin.h"
#include "plugins/step_sequencer_plugin.h"
#include "plugins/cv_to_control_plugin.h"
#include "plugins/control_to_cv_plugin.h"
#include "library/vst2x_wrapper.h"
#include "library/vst3x_wrapper.h"
#include "library/lv2/lv2_wrapper.h"


namespace sushi {
namespace engine {

constexpr auto RT_EVENT_TIMEOUT = std::chrono::milliseconds(200);
constexpr char TIMING_FILE_NAME[] = "timings.txt";
constexpr auto CLIPPING_DETECTION_INTERVAL = std::chrono::milliseconds(500);
constexpr int  MAX_TRACKS = 32;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("engine");


void ClipDetector::set_sample_rate(float samplerate)
{
    _interval = samplerate * CLIPPING_DETECTION_INTERVAL.count() / 1000 - AUDIO_CHUNK_SIZE;
}

void ClipDetector::set_input_channels(int channels)
{
    _input_clip_count = std::vector<unsigned int>(channels, _interval);
}

void ClipDetector::set_output_channels(int channels)
{
    _output_clip_count = std::vector<unsigned int>(channels, _interval);
}

void ClipDetector::detect_clipped_samples(const ChunkSampleBuffer& buffer, RtSafeRtEventFifo& queue, bool audio_input)
{
    auto& counter = audio_input? _input_clip_count : _output_clip_count;
    for (int i = 0; i < buffer.channel_count(); ++i)
    {
        if (buffer.count_clipped_samples(i, 1) > 0 && counter[i] >= _interval)
        {
            queue.push(RtEvent::make_clip_notification_event(0, i, audio_input? ClipNotificationRtEvent::ClipChannelType::INPUT:
                                                                   ClipNotificationRtEvent::ClipChannelType::OUTPUT));
            counter[i] = 0;
        }
        else
        {
            counter[i] += AUDIO_CHUNK_SIZE;
        }
    }
}

void external_render_callback(void* data)
{
    auto tracks = reinterpret_cast<std::vector<Track*>*>(data);
    for (auto i : *tracks)
    {
        i->render();
    }
}

AudioGraph::AudioGraph(int cpu_cores) : _audio_graph(cpu_cores),
                                        _event_outputs(cpu_cores),
                                        _cores(cpu_cores),
                                        _current_core(0)
{
    assert(cpu_cores > 0);
    if (_cores > 1)
    {
        _worker_pool = twine::WorkerPool::create_worker_pool(_cores);
        for (auto& i : _audio_graph)
        {
            _worker_pool->add_worker(external_render_callback, &i);
            i.reserve(MAX_TRACKS);
        }
    }
    else
    {
        _audio_graph[0].reserve(MAX_TRACKS);
    }
}

bool AudioGraph::add(Track* track)
{
    auto& slot = _audio_graph[_current_core];
    if (slot.size() < MAX_TRACKS)
    {
        track->set_event_output(&_event_outputs[_current_core]);
        slot.push_back(track);
        _current_core = (_current_core + 1) % _cores;
        return true;
    }
    return false;
}

bool AudioGraph::add_to_core(Track* track, int core)
{
    assert(core < _cores);
    auto& slot = _audio_graph[core];
    if (slot.size() < MAX_TRACKS)
    {
        track->set_event_output(&_event_outputs[core]);
        slot.push_back(track);
        return true;
    }
    return false;
}

bool AudioGraph::remove(Track* track)
{
    for (auto& slot : _audio_graph)
    {
        for (auto i = slot.begin(); i != slot.end(); ++i)
        {
            if (*i == track)
            {
                slot.erase(i);
                return true;
            }
        }
    }
    return false;
}

void AudioGraph::render()
{
    for (auto& i : _event_outputs)
    {
        i.clear();
    }

    if (_cores == 1)
    {
        for (auto& track : _audio_graph[0])
        {
            track->render();
        }
    }
    else
    {
        _worker_pool->wakeup_workers();
        _worker_pool->wait_for_workers_idle();
    }
}


AudioEngine::AudioEngine(float sample_rate, int rt_cpu_cores) : BaseEngine::BaseEngine(sample_rate),
                                                                _audio_graph(rt_cpu_cores),
                                                                _transport(sample_rate),
                                                                _clip_detector(sample_rate)
{
    this->set_sample_rate(sample_rate);
    _event_dispatcher.run();
}

AudioEngine::~AudioEngine()
{
    _event_dispatcher.stop();
    if (_process_timer.enabled())
    {
        _process_timer.enable(false);
        print_timings_to_file(TIMING_FILE_NAME);
    }
}

void AudioEngine::set_sample_rate(float sample_rate)
{
    BaseEngine::set_sample_rate(sample_rate);
    for (auto& node : _processors.all_processors())
    {
        _processors.mutable_processor(node->id())->configure(sample_rate);
    }
    _transport.set_sample_rate(sample_rate);
    _process_timer.set_timing_period(sample_rate, AUDIO_CHUNK_SIZE);
    _clip_detector.set_sample_rate(sample_rate);
}

void AudioEngine::set_audio_input_channels(int channels)
{
    _clip_detector.set_input_channels(channels);
    BaseEngine::set_audio_input_channels(channels);
}

void AudioEngine::set_audio_output_channels(int channels)
{
    _clip_detector.set_output_channels(channels);
    BaseEngine::set_audio_output_channels(channels);
}

EngineReturnStatus AudioEngine::set_cv_input_channels(int channels)
{
    if (channels > MAX_ENGINE_CV_IO_PORTS)
    {
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    return BaseEngine::set_cv_input_channels(channels);
}

EngineReturnStatus AudioEngine::set_cv_output_channels(int channels)
{
    if (channels > MAX_ENGINE_CV_IO_PORTS)
    {
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    return BaseEngine::set_cv_output_channels(channels);
}

EngineReturnStatus AudioEngine::connect_audio_input_channel(int input_channel, int track_channel, const std::string& track_name)
{
    auto track = _processors.track(track_name);
    if(track == nullptr)
    {
        return EngineReturnStatus::INVALID_TRACK;
    }
    if (input_channel >= _audio_inputs || track_channel >= track->input_channels())
    {
        return EngineReturnStatus::INVALID_CHANNEL;
    }
    AudioConnection con = {input_channel, track_channel, track->id()};
    _in_audio_connections.push_back(con);
    SUSHI_LOG_INFO("Connected inputs {} to channel {} of track \"{}\"", input_channel, track_channel, track_name);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_audio_output_channel(int output_channel, int track_channel,
                                                             const std::string& track_name)
{
    auto track = _processors.track(track_name);
    if(track == nullptr)
    {
        return EngineReturnStatus::INVALID_TRACK;
    }
    if (output_channel >= _audio_outputs || track_channel >= track->output_channels())
    {
        return EngineReturnStatus::INVALID_CHANNEL;
    }
    AudioConnection con = {output_channel, track_channel, track->id()};
    _out_audio_connections.push_back(con);
    SUSHI_LOG_INFO("Connected channel {} of track \"{}\" to output {}", track_channel, track_name, output_channel);
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

EngineReturnStatus AudioEngine::connect_cv_to_parameter(const std::string& processor_name,
                                                        const std::string& parameter_name,
                                                        int cv_input_id)
{
    if (cv_input_id >= _cv_inputs)
    {
        return EngineReturnStatus::INVALID_CHANNEL;
    }
    auto processor = _processors.mutable_processor(processor_name);
    if(processor == nullptr)
    {
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    auto param = processor->parameter_from_name(parameter_name);
    if (param == nullptr)
    {
        return EngineReturnStatus::INVALID_PARAMETER;
    }
    CvConnection con;
    con.processor_id = processor->id();
    con.parameter_id = param->id();
    con.cv_id = cv_input_id;
    _cv_in_routes.push_back(con);
    SUSHI_LOG_INFO("Connected cv input {} to parameter {} on {}", cv_input_id, parameter_name, processor_name);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_cv_from_parameter(const std::string& processor_name,
                                                          const std::string& parameter_name,
                                                          int cv_output_id)
{
    if (cv_output_id >= _cv_outputs)
    {
        return EngineReturnStatus::ERROR;
    }
    auto processor = _processors.mutable_processor(processor_name);
    if (processor == nullptr)
    {
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    auto param = processor->parameter_from_name(parameter_name);
    if (param == nullptr)
    {
        return EngineReturnStatus::INVALID_PARAMETER;
    }
    auto res = processor->connect_cv_from_parameter(param->id(), cv_output_id);
    if (res != ProcessorReturnCode::OK)
    {
        return EngineReturnStatus::ERROR;
    }
    SUSHI_LOG_INFO("Connected parameter {} on {} to cv output {}", parameter_name, processor_name, cv_output_id);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_gate_to_processor(const std::string& processor_name,
                                                          int gate_input_id,
                                                          int note_no,
                                                          int channel)
{
    if (gate_input_id >= MAX_ENGINE_GATE_PORTS || note_no > MAX_ENGINE_GATE_NOTE_NO)
    {
        return EngineReturnStatus::ERROR;
    }
    auto processor = _processors.mutable_processor(processor_name);
    if (processor == nullptr)
    {
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    GateConnection con;
    con.processor_id = processor->id();
    con.note_no = note_no;
    con.channel = channel;
    con.gate_id = gate_input_id;
    _gate_in_routes.push_back(con);
    SUSHI_LOG_INFO("Connected gate input {} to processor {} on channel {}", gate_input_id, processor_name, channel);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_gate_from_processor(const std::string& processor_name,
                                                            int gate_output_id,
                                                            int note_no,
                                                            int channel)
{
    if (gate_output_id >= MAX_ENGINE_GATE_PORTS || note_no > MAX_ENGINE_GATE_NOTE_NO)
    {
        return EngineReturnStatus::ERROR;
    }
    auto processor = _processors.mutable_processor(processor_name);
    if(processor == nullptr)
    {
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    auto res = processor->connect_gate_from_processor(gate_output_id, channel, note_no);
    if (res != ProcessorReturnCode::OK)
    {
        return EngineReturnStatus::ERROR;
    }
    SUSHI_LOG_INFO("Connected processor {} to gate output {} from channel {}", gate_output_id, processor_name, channel);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_gate_to_sync(int /*gate_input_id*/, int /*ppq_ticks*/)
{
    // TODO -  Needs implementing
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::connect_sync_to_gate(int /*gate_output_id*/, int /*ppq_ticks*/)
{
    // TODO -  Needs implementing
    return EngineReturnStatus::OK;
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
            _send_control_event(event);
        } else
        {
            _state.store(RealtimeState::STOPPED);
        }
    }
}

EngineReturnStatus AudioEngine::_register_processor(std::shared_ptr<Processor> processor, const std::string& name)
{
    if(name.empty())
    {
        SUSHI_LOG_ERROR("Plugin name is not specified");
        return EngineReturnStatus::INVALID_PLUGIN;
    }
    processor->set_name(name);
    if (_processors.add_processor(processor) == false)
    {
        SUSHI_LOG_WARNING("Processor with this name already exists");
        return EngineReturnStatus::INVALID_PROCESSOR;
    }
    SUSHI_LOG_DEBUG("Succesfully registered processor {}.", name);
    return EngineReturnStatus::OK;
}

void AudioEngine::_deregister_processor(Processor* processor)
{
    assert(processor);
    assert(processor->active_rt_processing() == false);
    _processors.remove_processor(processor->id());
    SUSHI_LOG_INFO("Successfully deregistered processor {}", processor->name());
}

bool AudioEngine::_insert_processor_in_realtime_part(Processor* processor)
{
    if (processor->id() > _realtime_processors.size())
    {
        /* TODO - When non-rt callbacks for events are ready we can have the
         * rt processor list re-allocated outside the rt domain
         * In the meantime, put a limit on the number of processors */
        SUSHI_LOG_ERROR("Realtime processor list full");
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

void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                                SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                                ControlBuffer* in_controls,
                                ControlBuffer* out_controls,
                                Time timestamp,
                                int64_t samplecount)
{
    /* Signal that this is a realtime audio processing thread */
    twine::ThreadRtFlag rt_flag;

    auto engine_timestamp = _process_timer.start_timer();

    _transport.set_time(timestamp, samplecount);

    _process_internal_rt_events();
    _send_rt_events_to_processors();

    if (_cv_inputs > 0)
    {
        _route_cv_gate_ins(*in_controls);
    }

    _event_dispatcher.set_time(_transport.current_process_time());
    auto state = _state.load();

    if (_input_clip_detection_enabled)
    {
        _clip_detector.detect_clipped_samples(*in_buffer, _main_out_queue, true);
    }
    _copy_audio_to_tracks(in_buffer);

    // Render all tracks
    _audio_graph.render();

    _retrieve_events_from_tracks(*out_controls);

    _main_out_queue.push(RtEvent::make_synchronisation_event(_transport.current_process_time()));
    _copy_audio_from_tracks(out_buffer);
    _state.store(update_state(state));

    if (_output_clip_detection_enabled)
    {
        _clip_detector.detect_clipped_samples(*out_buffer, _main_out_queue, false);
    }
    _process_timer.stop_timer(engine_timestamp, ENGINE_TIMING_ID);
}

void AudioEngine::set_tempo(float tempo)
{
    bool realtime_running = _state != RealtimeState::STOPPED;
    _transport.set_tempo(tempo, realtime_running);
    if (realtime_running)
    {
        auto e = RtEvent::make_tempo_event(0, tempo);
        _send_control_event(e);
    }
}

void AudioEngine::set_time_signature(TimeSignature signature)
{
    bool realtime_running = _state != RealtimeState::STOPPED;
    _transport.set_time_signature(signature, realtime_running);
    if (realtime_running)
    {
        auto e = RtEvent::make_time_signature_event(0, signature);
        _send_control_event(e);
    }
}

void AudioEngine::set_transport_mode(PlayingMode mode)
{
    bool realtime_running = _state != RealtimeState::STOPPED;
    _transport.set_playing_mode(mode, realtime_running);
    if (realtime_running)
    {
        auto e = RtEvent::make_playing_mode_event(0, mode);
        _send_control_event(e);
    }
}

void AudioEngine::set_tempo_sync_mode(SyncMode mode)
{
    bool realtime_running = _state != RealtimeState::STOPPED;
    _transport.set_sync_mode(mode, realtime_running);
    if (realtime_running)
    {
        auto e = RtEvent::make_sync_mode_event(0, mode);
        _send_control_event(e);
    }
}

EngineReturnStatus AudioEngine::send_rt_event(const RtEvent& event)
{
    auto status = _main_in_queue.push(event);
    return status? EngineReturnStatus::OK : EngineReturnStatus::QUEUE_FULL;
}

EngineReturnStatus AudioEngine::_send_control_event(RtEvent& event)
{
    // This queue will only handle engine control events, not processor events
    assert(event.type() >= RtEventType::STOP_ENGINE);
    std::lock_guard<std::mutex> lock(_in_queue_lock);
    if (_control_queue_in.push(event))
    {
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::QUEUE_FULL;
}

EngineReturnStatus AudioEngine::create_multibus_track(const std::string& name, int input_busses, int output_busses)
{
    if((input_busses > TRACK_MAX_BUSSES && output_busses > TRACK_MAX_BUSSES))
    {
        SUSHI_LOG_ERROR("Invalid number of busses for new track");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    auto track = std::make_shared<Track>(_host_control, input_busses, output_busses, &_process_timer);
    return _register_new_track(name, track);
}

EngineReturnStatus AudioEngine::create_track(const std::string &name, int channel_count)
{
    if((channel_count < 0 || channel_count > 2))
    {
        SUSHI_LOG_ERROR("Invalid number of channels for new track");
        return EngineReturnStatus::INVALID_N_CHANNELS;
    }
    auto track = std::make_shared<Track>(_host_control, channel_count, &_process_timer);
    return _register_new_track(name, track);
}

EngineReturnStatus AudioEngine::delete_track(const std::string &track_name)
{
    // TODO - Until it's decided how tracks report what processors they have,
    // we assume that the track has no processors before deleting
    auto track = _processors.mutable_track(track_name);
    if (track == nullptr)
    {
        SUSHI_LOG_ERROR("Couldn't delete track {}, not found", track_name);
        return EngineReturnStatus::INVALID_TRACK;
    }

    if (realtime())
    {
        auto remove_track_event = RtEvent::make_remove_track_event(track->id());
        auto delete_event = RtEvent::make_remove_processor_event(track->id());
        _send_control_event(remove_track_event);
        _send_control_event(delete_event);
        bool removed = _event_receiver.wait_for_response(remove_track_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool deleted = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!removed || !deleted)
        {
            SUSHI_LOG_ERROR("Failed to remove processor {} from processing part", track_name);
        }
    }
    else
    {
        _audio_graph.remove(track.get());
        [[maybe_unused]] bool removed = _remove_processor_from_realtime_part(track->id());
        SUSHI_LOG_WARNING_IF(removed == false,"Plugin track {} was not in the audio graph", track_name);
    }
    _processors.remove_track(track->id());
    _deregister_processor(track.get());
    return EngineReturnStatus::OK;
}

std::pair <EngineReturnStatus, ObjectId> AudioEngine::load_plugin(const std::string &plugin_uid,
                                                                  const std::string &plugin_name,
                                                                  const std::string &plugin_path,
                                                                  PluginType plugin_type)
{
    std::shared_ptr<Processor> plugin;
    switch (plugin_type)
    {
        case PluginType::INTERNAL:
            plugin = create_internal_plugin(plugin_uid, _host_control);
            if(plugin == nullptr)
            {
                SUSHI_LOG_ERROR("Unrecognised internal plugin \"{}\"", plugin_uid);
                return {EngineReturnStatus::INVALID_PLUGIN_UID, ObjectId(0)};
            }
            break;

        case PluginType::VST2X:
            plugin = std::make_shared<vst2::Vst2xWrapper>(_host_control, plugin_path);
            break;

        case PluginType::VST3X:
            plugin = std::make_shared<vst3::Vst3xWrapper>(_host_control, plugin_path, plugin_uid);
            break;

        case PluginType::LV2:
            plugin = std::make_shared<lv2::LV2_Wrapper>(_host_control, plugin_path);
            break;
    }

    auto processor_status = plugin->init(_sample_rate);
    if (processor_status != ProcessorReturnCode::OK)
    {
        SUSHI_LOG_ERROR("Failed to initialize plugin {}", plugin_name);
        return {EngineReturnStatus::INVALID_PLUGIN_UID, ObjectId(0)};
    }
    EngineReturnStatus status = _register_processor(plugin, plugin_name);
    if(status != EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to register plugin {}", plugin_name);
        return {status, ObjectId(0)};
    }

    plugin->set_enabled(true);
    if (this->realtime())
    {
        // In realtime mode we need to handle this in the audio thread
        auto insert_event = RtEvent::make_insert_processor_event(plugin.get());
        _send_control_event(insert_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted)
        {
            SUSHI_LOG_ERROR("Failed to insert plugin {} to processing part", plugin_name);
            _deregister_processor(plugin.get());
            return {EngineReturnStatus::INVALID_PROCESSOR, ObjectId(0)};
        }
    }
    else
    {
        // If the engine is not running in realtime mode we can add the processor directly
        _insert_processor_in_realtime_part(plugin.get());
    }
    return {EngineReturnStatus::OK, plugin->id()};
}

EngineReturnStatus AudioEngine::add_plugin_to_track(ObjectId plugin_id,
                                                    ObjectId track_id,
                                                    std::optional<ObjectId> before_plugin_id)
{
    auto track = _processors.mutable_track(track_id);
    if (track == nullptr)
    {
        SUSHI_LOG_ERROR("Track {} not found", track_id);
        return EngineReturnStatus::INVALID_TRACK;
    }

    auto plugin = _processors.mutable_processor(plugin_id);
    if (plugin == nullptr)
    {
        SUSHI_LOG_ERROR("Plugin {} not found", plugin_id);
        return EngineReturnStatus::INVALID_PLUGIN;
    }
    bool add_to_back = before_plugin_id.has_value() == false;

    if (add_to_back == false && _processors.processor_exists(before_plugin_id.value()) == false)
    {
        SUSHI_LOG_ERROR("Plugin {} not found", before_plugin_id.value());
        return EngineReturnStatus::INVALID_PLUGIN;
    }

    if (plugin->active_rt_processing())
    {
        SUSHI_LOG_ERROR("Plugin {} is already active on a track");
        return EngineReturnStatus::ERROR;
    }

    if (this->realtime())
    {
        // In realtime mode we need to handle this in the audio thread
        RtEvent add_event;
        if (add_to_back)
        {
            add_event = RtEvent::make_add_processor_to_track_back_event(plugin_id, track_id);
        }
        else
        {
            add_event = RtEvent::make_add_processor_to_track_event(plugin_id, track_id, before_plugin_id.value());
        }
        _send_control_event(add_event);
        bool added = _event_receiver.wait_for_response(add_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (added == false)
        {
            SUSHI_LOG_ERROR("Failed to add processor {} to track {}", plugin->name(), track->name());
            return EngineReturnStatus::INVALID_PROCESSOR;
        }
    }
    else
    {
        // If the engine is not running in realtime mode we can add the processor directly
        _insert_processor_in_realtime_part(plugin.get());
        bool added;
        if (add_to_back)
        {
            added = track->add_back(plugin.get());
        }
        else
        {
            added = track->add_before(plugin.get(), before_plugin_id.value());
        }
        if (added == false)
        {
            return EngineReturnStatus::ERROR;
        }
    }
    // Add it to the engine's mirror of track processing chains
    _processors.add_to_track(plugin, track->id(), before_plugin_id);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::remove_plugin_from_track(ObjectId plugin_id, ObjectId track_id)
{
    auto plugin = _processors.processor(plugin_id);
    auto track = _processors.mutable_track(track_id);
    if (plugin == nullptr)
    {
        return EngineReturnStatus::INVALID_PLUGIN;
    }
    if (track == nullptr)
    {
        return EngineReturnStatus::INVALID_TRACK;
    }

    if (realtime())
    {
        // Send events to handle this in the rt domain
        auto remove_event = RtEvent::make_remove_processor_from_track_event(plugin_id, track_id);
        _send_control_event(remove_event);
        [[maybe_unused]] bool remove_ok = _event_receiver.wait_for_response(remove_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        SUSHI_LOG_ERROR_IF(remove_ok == false, "Failed to remove/delete processor {} from processing part", plugin_id);
    }
    else
    {
        if (!track->remove(plugin.get()->id()))
        {
            SUSHI_LOG_ERROR("Failed to remove processor {} from track_id {}", plugin_id, track_id);
            return EngineReturnStatus::ERROR;
        }
    }

    _processors.remove_from_track(plugin_id, track_id);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::delete_plugin(ObjectId plugin_id)
{
    auto processor = _processors.mutable_processor(plugin_id);
    if (processor == nullptr)
    {
        return EngineReturnStatus::INVALID_PLUGIN;
    }
    if (processor->active_rt_processing())
    {
        SUSHI_LOG_ERROR("Cannot delete processor {}, active on track", processor->name());
        return EngineReturnStatus::ERROR;
    }
    if (realtime())
    {
        // Send events to handle this in the rt domain
        auto delete_event = RtEvent::make_remove_processor_event(processor->id());
        _send_control_event(delete_event);
        [[maybe_unused]] bool delete_ok = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        SUSHI_LOG_ERROR_IF(delete_ok == false, "Failed to remove/delete processor {} from processing part", plugin_id);
    }
    else
    {
        _remove_processor_from_realtime_part(processor->id());
    }

    _deregister_processor(processor.get());
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::_register_new_track(const std::string& name, std::shared_ptr<Track> track)
{
    track->init(_sample_rate);
    auto status = _register_processor(track, name);
    if (status != EngineReturnStatus::OK)
    {
        track.reset();
        return status;
    }

    if (realtime())
    {
        auto insert_event = RtEvent::make_insert_processor_event(track.get());
        auto add_event = RtEvent::make_add_track_event(track->id());
        _send_control_event(insert_event);
        _send_control_event(add_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool added = _event_receiver.wait_for_response(add_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted || !added)
        {
            SUSHI_LOG_ERROR("Failed to insert/add track {} to processing part", name);
            return EngineReturnStatus::INVALID_PROCESSOR;
        }
    }
    else
    {
        if (_audio_graph.add(track.get()) == false)
        {
            SUSHI_LOG_ERROR("Error adding track {}, max number of tracks reached", track->name());
            return EngineReturnStatus::ERROR;
        }
        if (_insert_processor_in_realtime_part(track.get()) == false)
        {
            SUSHI_LOG_ERROR("Error adding track {}", track->name());
            return EngineReturnStatus::ERROR;
        }
    }
    if (_processors.add_track(track))
    {
        SUSHI_LOG_INFO("Track {} successfully added to engine", name);
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::ERROR;
}

void AudioEngine::_process_internal_rt_events()
{
    RtEvent event;
    while(_control_queue_in.pop(event))
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
                Track*track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                Processor*processor = static_cast<Processor*>(_realtime_processors[typed_event->processor()]);
                if (track && processor)
                {
                    auto ok = track->add_back(processor);
                    typed_event->set_handled(ok);
                } else
                {
                    typed_event->set_handled(false);
                }
                break;
            }
            case RtEventType::REMOVE_PROCESSOR_FROM_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                Track*track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                if (track)
                {
                    bool ok = track->remove(typed_event->processor());
                    typed_event->set_handled(ok);
                } else
                {
                    typed_event->set_handled(true);
                }
                break;
            }
            case RtEventType::ADD_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                Track*track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                if (track)
                {
                    bool ok = _audio_graph.add(track);
                    if (ok)
                    {
                        track->set_active_rt_processing(true);
                        typed_event->set_handled(true);
                        break;
                    }
                }
                typed_event->set_handled(false);
                break;
            }
            case RtEventType::REMOVE_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                Track*track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                if (track)
                {
                    bool removed = _audio_graph.remove(track);
                    if (removed)
                    {
                        track->set_active_rt_processing(false);
                        typed_event->set_handled(true);
                        break;
                    }
                }
                typed_event->set_handled(false);
                break;
            }
            case RtEventType::TEMPO:
            case RtEventType::TIME_SIGNATURE:
            case RtEventType::PLAYING_MODE:
            case RtEventType::SYNC_MODE:
            {
                _transport.process_event(event);
                break;
            }

            default:
                break;
        }
        _control_queue_out.push(event); // Send event back to non-rt domain
    }
}

void AudioEngine::_send_rt_events_to_processors()
{
    RtEvent event;
    while(_main_in_queue.pop(event))
    {
        _send_rt_event(event);
    }
}

void AudioEngine::_send_rt_event(const RtEvent& event)
{
    if (event.processor_id() < _realtime_processors.size() &&
        _realtime_processors[event.processor_id()] != nullptr)
    {
        _realtime_processors[event.processor_id()]->process_event(event);
    }
}


void AudioEngine::_retrieve_events_from_tracks(ControlBuffer& buffer)
{
    for (auto& output : _audio_graph.event_outputs())
    {
        while (output.empty() == false)
        {
            const RtEvent& event = output.pop();
            switch (event.type())
            {
                case RtEventType::CV_EVENT:
                {
                    auto typed_event = event.cv_event();
                    buffer.cv_values[typed_event->cv_id()] = typed_event->value();
                    break;
                }

                case RtEventType::GATE_EVENT:
                {
                    auto typed_event = event.gate_event();
                    _outgoing_gate_values[typed_event->gate_no()] = typed_event->value();
                    break;
                }

                default:
                    _main_out_queue.push(event);
            }
        }
        buffer.gate_values = _outgoing_gate_values;
    }
}

void AudioEngine::_copy_audio_to_tracks(ChunkSampleBuffer* input)
{
    for (const auto& c : _in_audio_connections)
    {
        auto engine_in = ChunkSampleBuffer::create_non_owning_buffer(*input, c.engine_channel, 1);
        auto track_in = static_cast<Track*>(_realtime_processors[c.track])->input_channel(c.track_channel);
        track_in = engine_in;
    }
}

void AudioEngine::_copy_audio_from_tracks(ChunkSampleBuffer* output)
{
    output->clear();
    for (const auto& c : _out_audio_connections)
    {
        auto track_out = static_cast<Track*>(_realtime_processors[c.track])->output_channel(c.track_channel);
        auto engine_out = ChunkSampleBuffer::create_non_owning_buffer(*output, c.engine_channel, 1);
        engine_out.add(track_out);
    }
}

void AudioEngine::print_timings_to_log()
{
    if (_process_timer.enabled())
    {
        for (const auto& processor : _processors.all_processors())
        {
            auto id = processor->id();
            auto timings = _process_timer.timings_for_node(id);
            if (timings.has_value())
            {
                SUSHI_LOG_INFO("Processor: {} ({}), avg: {}%, min: {}%, max: {}%", id, processor->name(),
                              timings->avg_case * 100.0f, timings->min_case * 100.0f, timings->max_case * 100.0f);
            }
        }
        auto timings = _process_timer.timings_for_node(ENGINE_TIMING_ID);
        if (timings.has_value())
        {
            SUSHI_LOG_INFO("Engine total: avg: {}%, min: {}%, max: {}%",
                          timings->avg_case * 100.0f, timings->min_case * 100.0f, timings->max_case * 100.0f);
        }
    }
}


void print_single_timings_for_node(std::fstream& f, performance::PerformanceTimer& timer, int id)
{
    auto timings = timer.timings_for_node(id);
    if (timings.has_value())
    {
        f << std::setw(16) << timings.value().avg_case * 100.0
          << std::setw(16) << timings.value().min_case * 100.0
          << std::setw(16) << timings.value().max_case * 100.0 <<"\n";
    }
}

void AudioEngine::print_timings_to_file(const std::string& filename)
{
    std::fstream file;
    file.open(filename, std::ios_base::out);
    if (!file.is_open())
    {
        SUSHI_LOG_WARNING("Couldn't write timings to file");
        return;
    }
    file.setf(std::ios::left);
    file << "Performance timings for all processors in percentages of audio buffer (100% = "<< 1000000.0 / _sample_rate * AUDIO_CHUNK_SIZE
         << "us)\n\n" << std::setw(24) << "" << std::setw(16) << "average(%)" << std::setw(16) << "minimum(%)"
         << std::setw(16) << "maximum(%)" << std::endl;

    for (const auto& track : _processors.all_tracks())
    {
        file << std::setw(0) << "Track: " << track->name() << "\n";
        for (auto& p : _processors.processors_on_track(track->id()))
        {
            file << std::setw(8) << "" << std::setw(16) << p->name();
            print_single_timings_for_node(file, _process_timer, p->id());
        }
        file << std::setw(8) << "" << std::setw(16) << "Track total";
        print_single_timings_for_node(file, _process_timer, track->id());
        file << "\n";
    }

    file << std::setw(24) << "Engine total";
    print_single_timings_for_node(file, _process_timer, ENGINE_TIMING_ID);
    file.close();
}

void AudioEngine::_route_cv_gate_ins(ControlBuffer& buffer)
{
    for (const auto& r : _cv_in_routes)
    {
        float value = buffer.cv_values[r.cv_id];
        auto ev = RtEvent::make_parameter_change_event(r.processor_id, 0, r.parameter_id, value);
        _send_rt_event(ev);
    }
    // Get gate state changes by xor:ing with previous states
    auto gate_diffs = _prev_gate_values ^ buffer.gate_values;
    if (gate_diffs.any())
    {
        for (const auto& r : _gate_in_routes)
        {
            if (gate_diffs[r.gate_id])
            {
                auto gate_high = buffer.gate_values[r.gate_id];
                if (gate_high)
                {
                    auto ev = RtEvent::make_note_on_event(r.processor_id, 0, r.channel, r.note_no, 1.0f);
                    _send_rt_event(ev);
                }
                else
                {
                    auto ev = RtEvent::make_note_off_event(r.processor_id, 0, r.channel, r.note_no, 1.0f);
                    _send_rt_event(ev);
                }
            }
        }
    }
    _prev_gate_values = buffer.gate_values;
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

std::shared_ptr<Processor> create_internal_plugin(const std::string& uid, HostControl& host_control)
{
    if (uid == "sushi.testing.passthrough")
    {
        return std::make_shared<passthrough_plugin::PassthroughPlugin>(host_control);
    }
    else if (uid == "sushi.testing.gain")
    {
        return std::make_shared<gain_plugin::GainPlugin>(host_control);
    }
    else if (uid == "sushi.testing.lfo")
    {
        return std::make_shared<lfo_plugin::LfoPlugin>(host_control);
    }
    else if (uid == "sushi.testing.equalizer")
    {
        return std::make_shared<equalizer_plugin::EqualizerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.sampleplayer")
    {
        return std::make_shared<sample_player_plugin::SamplePlayerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.arpeggiator")
    {
        return std::make_shared<arpeggiator_plugin::ArpeggiatorPlugin>(host_control);
    }
    else if (uid == "sushi.testing.peakmeter")
    {
        return std::make_shared<peak_meter_plugin::PeakMeterPlugin>(host_control);
    }
    else if (uid == "sushi.testing.transposer")
    {
        return std::make_shared<transposer_plugin::TransposerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.step_sequencer")
    {
        return std::make_shared<step_sequencer_plugin::StepSequencerPlugin>(host_control);
    }
    else if (uid == "sushi.testing.cv_to_control")
    {
        return std::make_shared<cv_to_control_plugin::CvToControlPlugin>(host_control);
    }
    else if (uid == "sushi.testing.control_to_cv")
    {
        return std::make_shared<control_to_cv_plugin::ControlToCvPlugin>(host_control);
    }
    return nullptr;
}

bool ProcessorContainer::add_processor(std::shared_ptr<Processor> processor)
{
    std::unique_lock<std::mutex> name_lock(_processors_by_name_lock);
    std::unique_lock<std::mutex> id_lock(_processors_by_id_lock);
    if (_processors_by_name.count(processor->name()) > 0)
    {
        return false;
    }
    _processors_by_name[processor->name()] = processor;
    _processors_by_id[processor->id()] = processor;
    return true;
}

bool ProcessorContainer::add_track(std::shared_ptr<Track> track)
{
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    if (_processors_by_track.count(track->id()) > 0)
    {
        return false;
    }
    _processors_by_track[track->id()].clear();
    return true;
}

bool ProcessorContainer::remove_processor(ObjectId id)
{
    auto processor = this->processor(id);
    if (processor == nullptr)
    {
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(_processors_by_id_lock);
        [[maybe_unused]] auto count = _processors_by_id.erase(processor->id());
        SUSHI_LOG_WARNING_IF(count != 1, "Erased {} instances of processor {}", count, processor->name());
    }
    {
        std::unique_lock<std::mutex> lock(_processors_by_name_lock);
        [[maybe_unused]] auto count = _processors_by_name.erase(processor->name());
        SUSHI_LOG_WARNING_IF(count != 1, "Erased {} instances of processor {}", count, processor->name());
    }
    return true;
}

bool ProcessorContainer::remove_track(ObjectId track_id)
{
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    assert(_processors_by_track[track_id].size() == 0);
    _processors_by_track.erase(track_id);
    return true;
}

bool ProcessorContainer::add_to_track(std::shared_ptr<Processor> processor, ObjectId track_id,
                                      std::optional<ObjectId> before_id)
{
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    auto& track_processors = _processors_by_track[track_id];

    if (before_id.has_value())
    {
        for (auto i = track_processors.begin(); i != track_processors.end(); ++i)
        {
            if ((*i)->id() == before_id.value())
            {
                track_processors.insert(i, processor);
                return true;
            }
        }
        // If we end up here, the track's processing chain and _processors_by_track has diverged.
        assert(false);
    }
    else
    {
        track_processors.push_back(processor);
    }
    return true;
}

bool ProcessorContainer::processor_exists(ObjectId id) const
{
    std::unique_lock<std::mutex> lock(_processors_by_id_lock);
    return _processors_by_id.count(id) > 0;
}

bool ProcessorContainer::processor_exists(const std::string& name) const
{
    std::unique_lock<std::mutex> lock(_processors_by_name_lock);
    return _processors_by_name.count(name) > 0;
}

bool ProcessorContainer::remove_from_track(ObjectId processor_id, ObjectId track_id)
{
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    auto& track_processors = _processors_by_track[track_id];
    for (auto i = track_processors.cbegin(); i != track_processors.cend(); ++i)
    {
        if ((*i)->id() == processor_id)
        {
            track_processors.erase(i);
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<const Processor>> ProcessorContainer::all_processors() const
{
    std::vector<std::shared_ptr<const Processor>> processors;
    std::unique_lock<std::mutex> lock(_processors_by_id_lock);
    processors.reserve(_processors_by_id.size());
    for (const auto& p : _processors_by_id)
    {
        processors.emplace_back(p.second);
    }
    return processors;
}

std::shared_ptr<Processor> ProcessorContainer::mutable_processor(ObjectId id) const
{
    return std::const_pointer_cast<Processor>(this->processor(id));
}

std::shared_ptr<Processor> ProcessorContainer::mutable_processor(const std::string& name) const
{
    return std::const_pointer_cast<Processor>(this->processor(name));
}

std::shared_ptr<const Processor> ProcessorContainer::processor(ObjectId id) const
{
    std::unique_lock<std::mutex> lock(_processors_by_id_lock);
    auto processor_node = _processors_by_id.find(id);
    if (processor_node == _processors_by_id.end())
    {
        return nullptr;
    }
    return processor_node->second;
}

std::shared_ptr<const Processor> ProcessorContainer::processor(const std::string& name) const
{
    std::unique_lock<std::mutex> lock(_processors_by_name_lock);
    auto processor_node = _processors_by_name.find(name);
    if (processor_node == _processors_by_name.end())
    {
        return nullptr;
    }
    return processor_node->second;
}

std::shared_ptr<Track> ProcessorContainer::mutable_track(ObjectId track_id) const
{
    return std::const_pointer_cast<Track>(this->track(track_id));
}

std::shared_ptr<Track> ProcessorContainer::mutable_track(const std::string& track_name) const
{
    return std::const_pointer_cast<Track>(this->track(track_name));
}

std::shared_ptr<const Track> ProcessorContainer::track(ObjectId track_id) const
{
    /* Check if there is an entry for the ObjectId in the list of track processor
     * In that case we can safely look up the processor by its id and cast it */
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    if (_processors_by_track.count(track_id) > 0)
    {
        std::unique_lock<std::mutex> id_lock(_processors_by_id_lock);
        auto track_node = _processors_by_id.find(track_id);
        if (track_node != _processors_by_id.end())
        {
            return std::static_pointer_cast<const Track>(track_node->second);
        }
    }
    return nullptr;
}

std::shared_ptr<const Track> ProcessorContainer::track(const std::string& track_name) const
{
    std::unique_lock<std::mutex> _lock(_processors_by_name_lock);
    auto track_node = _processors_by_name.find(track_name);
    if (track_node != _processors_by_name.end())
    {
        /* Check if there is an entry for the ObjectId in the list of track processor
         * In that case we can safely look up the processor by its id and cast it */
        std::unique_lock<std::mutex> lock(_processors_by_track_lock);
        if (_processors_by_track.count(track_node->second->id()) > 0)
        {
            return std::static_pointer_cast<const Track>(track_node->second);
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<const Processor>> ProcessorContainer::processors_on_track(ObjectId track_id) const
{
    std::vector<std::shared_ptr<const Processor>> processors;
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    auto track_node = _processors_by_track.find(track_id);
    if (track_node != _processors_by_track.end())
    {
        processors.reserve(track_node->second.size());
        for (const auto& p : track_node->second)
        {
            processors.push_back(p);
        }
    }
    return processors;
}

std::vector<std::shared_ptr<const Track>> ProcessorContainer::all_tracks() const
{
    std::vector<std::shared_ptr<const Track>> tracks;
    std::unique_lock<std::mutex> lock(_processors_by_track_lock);
    for (const auto& p : _processors_by_track)
    {
        std::unique_lock<std::mutex> id_lock(_processors_by_id_lock);
        auto processor_node = _processors_by_id.find(p.first);
        if (processor_node != _processors_by_id.end())
        {
            tracks.push_back(std::static_pointer_cast<const Track, Processor>(processor_node->second));
        }
    }
    lock.unlock();
    /* Sort the list so tracks are listed in the order they were created */
    std::sort(tracks.begin(), tracks.end(), [](auto a, auto b) {return a->id() < b->id();});
    return tracks;
}

} // namespace engine
} // namespace sushi
