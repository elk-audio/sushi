/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <fstream>
#include <iomanip>
#include <functional>

#include "twine/src/twine_internal.h"

#include "audio_engine.h"
#include "logging.h"


namespace sushi {
namespace engine {

constexpr auto CLIPPING_DETECTION_INTERVAL = std::chrono::milliseconds(500);

constexpr auto RT_EVENT_TIMEOUT = std::chrono::milliseconds(200);
constexpr char TIMING_FILE_NAME[] = "timings.txt";
constexpr int  TIMING_LOG_PRINT_INTERVAL = 15;

constexpr int  MAX_TRACKS = 32;
constexpr int  MAX_AUDIO_CONNECTIONS = MAX_TRACKS * MAX_TRACK_CHANNELS;
constexpr int  MAX_CV_CONNECTIONS = MAX_ENGINE_CV_IO_PORTS * 10;
constexpr int  MAX_GATE_CONNECTIONS = MAX_ENGINE_GATE_PORTS * 10;

SUSHI_GET_LOGGER_WITH_MODULE_NAME("engine");

EngineReturnStatus to_engine_status(ProcessorReturnCode processor_status)
{
    switch (processor_status)
    {
        case ProcessorReturnCode::OK:                       return EngineReturnStatus::OK;
        case ProcessorReturnCode::ERROR:                    return EngineReturnStatus::ERROR;
        case ProcessorReturnCode::PARAMETER_ERROR:          return EngineReturnStatus::INVALID_PARAMETER;
        case ProcessorReturnCode::PARAMETER_NOT_FOUND:      return EngineReturnStatus::INVALID_PARAMETER;
        case ProcessorReturnCode::UNSUPPORTED_OPERATION:    return EngineReturnStatus::INVALID_PLUGIN_TYPE;
        default:                                            return EngineReturnStatus::ERROR;
    }
}


void ClipDetector::set_sample_rate(float samplerate)
{
    _interval = static_cast<unsigned int>(samplerate * CLIPPING_DETECTION_INTERVAL.count() / 1000 - AUDIO_CHUNK_SIZE);
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
        if (buffer.count_clipped_samples(i) > 0 && counter[i] >= _interval)
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

AudioEngine::AudioEngine(float sample_rate,
                         int rt_cpu_cores,
                         bool debug_mode_sw,
                         dispatcher::BaseEventDispatcher* event_dispatcher) : BaseEngine::BaseEngine(sample_rate),
                                                          _audio_graph(rt_cpu_cores, MAX_TRACKS, debug_mode_sw),
                                                          _audio_in_connections(MAX_AUDIO_CONNECTIONS),
                                                          _audio_out_connections(MAX_AUDIO_CONNECTIONS),
                                                          _transport(sample_rate, &_main_out_queue),
                                                          _clip_detector(sample_rate)
{
    if (event_dispatcher == nullptr)
    {
        _event_dispatcher = std::make_unique<dispatcher::EventDispatcher>(this, &_main_out_queue, &_main_in_queue);
    }
    else
    {
        _event_dispatcher.reset(event_dispatcher);
    }
    _host_control = HostControl(_event_dispatcher.get(), &_transport, &_plugin_library);

    this->set_sample_rate(sample_rate);
    _cv_in_connections.reserve(MAX_CV_CONNECTIONS);
    _gate_in_connections.reserve(MAX_GATE_CONNECTIONS);
}

AudioEngine::~AudioEngine()
{
    _event_dispatcher->stop();
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
    for (auto& limiter : _master_limiters)
    {
        limiter.init(sample_rate);
    }
}

void AudioEngine::set_audio_input_channels(int channels)
{
    _clip_detector.set_input_channels(channels);
    BaseEngine::set_audio_input_channels(channels);
    _input_swap_buffer = ChunkSampleBuffer(channels);
}

void AudioEngine::set_audio_output_channels(int channels)
{
    _clip_detector.set_output_channels(channels);
    BaseEngine::set_audio_output_channels(channels);
    _master_limiters.clear();
    for (int c = 0; c < channels; c++)
    {
        _master_limiters.emplace_back();
    }
    _output_swap_buffer = ChunkSampleBuffer(channels);
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

EngineReturnStatus AudioEngine::connect_audio_input_channel(int input_channel, int track_channel, ObjectId track_id)
{
    return _connect_audio_channel(input_channel, track_channel, track_id, Direction::INPUT);
}

EngineReturnStatus AudioEngine::connect_audio_output_channel(int output_channel, int track_channel, ObjectId track_id)
{
    return _connect_audio_channel(output_channel, track_channel, track_id, Direction::OUTPUT);
}

EngineReturnStatus AudioEngine::disconnect_audio_input_channel(int engine_channel, int track_channel, ObjectId track_name)
{
    return _disconnect_audio_channel(engine_channel, track_channel, track_name, Direction::INPUT);
}

EngineReturnStatus AudioEngine::disconnect_audio_output_channel(int engine_channel, int track_channel, ObjectId track_name)
{
    return _disconnect_audio_channel(engine_channel, track_channel, track_name, Direction::OUTPUT);
}

std::vector<AudioConnection> AudioEngine::audio_input_connections()
{
    return _audio_in_connections.connections();
}

std::vector<AudioConnection> AudioEngine::audio_output_connections()
{
    return _audio_out_connections.connections();
}

EngineReturnStatus AudioEngine::connect_audio_input_bus(int input_bus, int track_bus, ObjectId track_id)
{
    auto status = connect_audio_input_channel(input_bus * 2, track_bus * 2, track_id);
    if (status != EngineReturnStatus::OK)
    {
        return status;
    }
    return connect_audio_input_channel(input_bus * 2 + 1, track_bus * 2 + 1, track_id);
}

EngineReturnStatus AudioEngine::connect_audio_output_bus(int output_bus, int track_bus, ObjectId track_id)
{
    auto status = connect_audio_output_channel(output_bus * 2, track_bus * 2, track_id);
    if (status != EngineReturnStatus::OK)
    {
        return status;
    }
    return connect_audio_output_channel(output_bus * 2 + 1, track_bus * 2 + 1, track_id);
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
    if (processor == nullptr)
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
    _cv_in_connections.push_back(con);
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
    _gate_in_connections.push_back(con);
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
    if (processor == nullptr)
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
        _state.store(RealtimeState::STOPPED);
    }
}

EngineReturnStatus AudioEngine::_register_processor(std::shared_ptr<Processor> processor, const std::string& name)
{
    if (name.empty())
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
    SUSHI_LOG_DEBUG("Successfully registered processor {}.", name);
    return EngineReturnStatus::OK;
}

void AudioEngine::_deregister_processor(Processor* processor)
{
    assert(processor);
    assert(processor->active_rt_processing() == false);
    _processors.remove_processor(processor->id());
    SUSHI_LOG_INFO("Successfully de-registered processor {}", processor->name());
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
    if (_realtime_processors[processor->id()] != nullptr)
    {
        return false;
    }
    _realtime_processors[processor->id()] = processor;
    return true;
}

bool AudioEngine::_remove_processor_from_realtime_part(ObjectId processor)
{
    if (_realtime_processors[processor] == nullptr)
    {
        return false;
    }
    _realtime_processors[processor] = nullptr;
    return true;
}

void AudioEngine::_remove_connections_from_track(ObjectId track_id)
{
    for (auto con : _audio_out_connections.connections())
    {
        if (con.track == track_id)
        {
            this->disconnect_audio_output_channel(con.engine_channel, con.track_channel, con.track);
        }
    }
    for (auto con : _audio_in_connections.connections())
    {
        if (con.track == track_id)
        {
            this->disconnect_audio_input_channel(con.engine_channel, con.track_channel, con.track);
        }
    }
}

void AudioEngine::process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                                SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                                ControlBuffer* in_controls,
                                ControlBuffer* out_controls,
                                Time timestamp,
                                int64_t sample_count)
{
    /* Signal that this is a realtime audio processing thread */
    twine::ThreadRtFlag rt_flag;

    auto engine_timestamp = _process_timer.start_timer();

    _transport.set_time(timestamp, sample_count);

    _process_internal_rt_events();
    _send_rt_events_to_processors();

    if (_cv_inputs > 0)
    {
        _route_cv_gate_ins(*in_controls);
    }

    _event_dispatcher->set_time(_transport.current_process_time());
    auto state = _state.load();

    if (_input_clip_detection_enabled)
    {
        _clip_detector.detect_clipped_samples(*in_buffer, _main_out_queue, true);
    }

    if (_pre_track)
    {
        _pre_track->process_audio(*in_buffer, _input_swap_buffer);
        _copy_audio_to_tracks(&_input_swap_buffer);
    }
    else
    {
        _copy_audio_to_tracks(in_buffer);
    }

    // Render all tracks. If running in multicore mode, this part is processed in parallel.
    _audio_graph.render();

    _retrieve_events_from_tracks(*out_controls);
    _main_out_queue.push(RtEvent::make_synchronisation_event(_transport.current_process_time()));
    _state.store(update_state(state));

    if (_post_track)
    {
        _copy_audio_from_tracks(&_output_swap_buffer);
        _post_track->process_audio(_output_swap_buffer, *out_buffer);
    }
    else
    {
        _copy_audio_from_tracks(out_buffer);
    }

    if (_master_limiter_enabled)
    {
        auto temp_input_buffer = ChunkSampleBuffer::create_non_owning_buffer(*out_buffer, 0, out_buffer->channel_count());
        for (int c = 0; c < out_buffer->channel_count(); c++)
        {
            _master_limiters[c].process(out_buffer->channel(c), out_buffer->channel(c));
        }
        out_buffer->replace(temp_input_buffer);
    }

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
    assert(is_engine_control_event(event));
    std::lock_guard<std::mutex> lock(_in_queue_lock);
    if (_control_queue_in.push(event))
    {
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::QUEUE_FULL;
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::create_multibus_track(const std::string& name, int bus_count)
{
    if (bus_count > MAX_TRACK_BUSES)
    {
        SUSHI_LOG_ERROR("Invalid number of buses for new track");
        return {EngineReturnStatus::INVALID_N_CHANNELS, ObjectId(0)};
    }
    auto track = std::make_shared<Track>(_host_control, bus_count, &_process_timer);
    auto status = _register_new_track(name, track);
    if (status != EngineReturnStatus::OK)
    {
        return {status, ObjectId(0)};
    }
    return {EngineReturnStatus::OK, track->id()};
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::create_track(const std::string &name, int channel_count)
{
    if ((channel_count < 0 || channel_count > MAX_TRACK_CHANNELS))
    {
        SUSHI_LOG_ERROR("Invalid number of channels for new track");
        return {EngineReturnStatus::INVALID_N_CHANNELS, ObjectId(0)};
    }
    // Only mono and stereo track have a pan parameter
    bool pan_control = channel_count <= 2;

    auto track = std::make_shared<Track>(_host_control, channel_count, &_process_timer, pan_control);
    auto status = _register_new_track(name, track);
    if (status != EngineReturnStatus::OK)
    {
        return {status, ObjectId(0)};
    }
    return {EngineReturnStatus::OK, track->id()};
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::create_post_track(const std::string& name)
{
    return _create_master_track(name, TrackType::POST, _audio_outputs);
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::create_pre_track(const std::string& name)
{
    return _create_master_track(name, TrackType::PRE, _audio_inputs);
}

EngineReturnStatus AudioEngine::delete_track(ObjectId track_id)
{
    auto track = _processors.mutable_track(track_id);
    if (track == nullptr)
    {
        SUSHI_LOG_ERROR("Couldn't delete track {}, not found", track_id);
        return EngineReturnStatus::INVALID_TRACK;
    }
    if (_processors.processors_on_track(track->id()).empty() == false)
    {
        SUSHI_LOG_ERROR("Couldn't delete track {}, track not empty", track_id);
        return EngineReturnStatus::ERROR;
    }

    // First remove any audio connections, if realtime, this is done with RtEvents
    _remove_connections_from_track(track->id());

    if (realtime())
    {
        auto remove_event = RtEvent::make_remove_track_event(track->id());
        auto delete_event = RtEvent::make_remove_processor_event(track->id());
        _send_control_event(remove_event);
        _send_control_event(delete_event);
        bool removed = _event_receiver.wait_for_response(remove_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        bool deleted = _event_receiver.wait_for_response(delete_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!removed || !deleted)
        {
            SUSHI_LOG_ERROR("Failed to remove processor {} from processing part", track->name());
        }
    }
    else
    {
        _remove_track(track.get());
        [[maybe_unused]] bool removed = _remove_processor_from_realtime_part(track->id());
        SUSHI_LOG_WARNING_IF(removed == false, "Plugin track {} was not in the audio graph", track_id)
    }
    track->set_enabled(false);
    _processors.remove_track(track->id());
    _deregister_processor(track.get());
    _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_DELETED,
                                                                      0,
                                                                  track->id(),
                                                                  IMMEDIATE_PROCESS));
    return EngineReturnStatus::OK;
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::create_processor(const PluginInfo& plugin_info, const std::string &processor_name)
{
    auto [processor_status, processor] = _plugin_registry.new_instance(plugin_info, _host_control, _sample_rate);

    if (processor_status != ProcessorReturnCode::OK)
    {
        SUSHI_LOG_ERROR("Failed to initialize processor {} with error {}", processor_name, static_cast<int>(processor_status));
        return {to_engine_status(processor_status), ObjectId(0)};
    }
    EngineReturnStatus status = _register_processor(processor, processor_name);
    if (status != EngineReturnStatus::OK)
    {
        SUSHI_LOG_ERROR("Failed to register processor {}", processor_name);
        return {status, ObjectId(0)};
    }

    if (this->realtime())
    {
        // In realtime mode we need to handle this in the audio thread
        auto insert_event = RtEvent::make_insert_processor_event(processor.get());
        _send_control_event(insert_event);
        bool inserted = _event_receiver.wait_for_response(insert_event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (!inserted)
        {
            SUSHI_LOG_ERROR("Failed to insert processor {} to processing part", processor_name);
            _deregister_processor(processor.get());
            return {EngineReturnStatus::INVALID_PROCESSOR, ObjectId(0)};
        }
    }
    else
    {
        // If the engine is not running in realtime mode we can add the processor directly
        _insert_processor_in_realtime_part(processor.get());
    }
    _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_CREATED,
                                                                  processor->id(),
                                                                  0,
                                                                  IMMEDIATE_PROCESS));
    return {EngineReturnStatus::OK, processor->id()};
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

    if (plugin->active_rt_processing())
    {
        SUSHI_LOG_ERROR("Plugin {} is already active on a track");
        return EngineReturnStatus::ERROR;
    }

    plugin->set_enabled(true);
    plugin->set_input_channels(std::min(plugin->max_input_channels(), track->input_channels()));
    plugin->set_output_channels(std::min(plugin->max_output_channels(), track->input_channels()));

    if (this->realtime())
    {
        // In realtime mode we need to handle this in the audio thread
        RtEvent add_event = RtEvent::make_add_processor_to_track_event(plugin_id, track_id, before_plugin_id);
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
        bool added = track->add(plugin.get(), before_plugin_id);
        if (added == false)
        {
            return EngineReturnStatus::ERROR;
        }
    }
    // Add it to the engine's mirror of track processing chains
    _processors.add_to_track(plugin, track->id(), before_plugin_id);
    _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_ADDED_TO_TRACK,
                                                                  plugin_id,
                                                                  track_id,
                                                                  IMMEDIATE_PROCESS));
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::remove_plugin_from_track(ObjectId plugin_id, ObjectId track_id)
{
    auto plugin = _processors.mutable_processor(plugin_id);
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
        SUSHI_LOG_ERROR_IF(remove_ok == false, "Failed to remove/delete processor {} from processing part", plugin_id)
    }
    else
    {
        if (!track->remove(plugin->id()))
        {
            SUSHI_LOG_ERROR("Failed to remove processor {} from track_id {}", plugin_id, track_id);
            return EngineReturnStatus::ERROR;
        }
    }

    plugin->set_enabled(false);

    bool removed = _processors.remove_from_track(plugin_id, track_id);
    if (removed)
    {
        _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_REMOVED_FROM_TRACK,
                                                                      plugin_id,
                                                                      track_id,
                                                                      IMMEDIATE_PROCESS));
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::ERROR;
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
        SUSHI_LOG_ERROR_IF(delete_ok == false, "Failed to remove/delete processor {} from processing part", plugin_id)
    }
    else
    {
        _remove_processor_from_realtime_part(processor->id());
    }

    _deregister_processor(processor.get());
    _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::PROCESSOR_DELETED,
                                                                  processor->id(),
                                                                  0,
                                                                  IMMEDIATE_PROCESS));
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::_register_new_track(const std::string& name, std::shared_ptr<Track> track)
{
    track->init(_sample_rate);
    track->set_enabled(true);

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
        if (_add_track(track.get()) == false)
        {

            SUSHI_LOG_ERROR_IF(track->type() == TrackType::REGULAR, "Error adding track {}, max number of tracks reached", track->name());
            SUSHI_LOG_ERROR_IF(track->type() == TrackType::PRE, "Error adding track {}, Only one pre track allowed", track->name());
            SUSHI_LOG_ERROR_IF(track->type() == TrackType::POST, "Error adding track {}, Only one post track allowed", track->name());
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
        _event_dispatcher->post_event(new AudioGraphNotificationEvent(AudioGraphNotificationEvent::Action::TRACK_CREATED,
                                                                      0,
                                                                      track->id(),
                                                                      IMMEDIATE_PROCESS));
        return EngineReturnStatus::OK;
    }
    return EngineReturnStatus::ERROR;
}

std::pair<EngineReturnStatus, ObjectId> AudioEngine::_create_master_track(const std::string& name, TrackType type, int channels)
{
    auto track = std::make_shared<Track>(_host_control, channels, &_process_timer, false, type);
    auto status = _register_new_track(name, track);
    if (status != EngineReturnStatus::OK)
    {
        return {status, ObjectId(0)};
    }
    return {EngineReturnStatus::OK, track->id()};
}

EngineReturnStatus AudioEngine::_connect_audio_channel(int engine_channel,
                                                       int track_channel,
                                                       ObjectId track_id,
                                                       Direction direction)
{
    auto track = _processors.mutable_track(track_id);
    if (track == nullptr)
    {
        return EngineReturnStatus::INVALID_TRACK;
    }

    if (direction == Direction::INPUT)
    {
        if (engine_channel >= _audio_inputs || track_channel >= track->input_channels())
        {
            return EngineReturnStatus::INVALID_CHANNEL;
        }
    }
    else
    {
        if (engine_channel >= _audio_outputs || track_channel >= track->output_channels())
        {
            if (track_channel == 1 &&
                track->max_output_channels() == 2 &&
                track->output_channels() <= 1)
            {
                // Corner case when connecting a mono track to a stereo output bus, this is allowed
                track->set_output_channels(2);
            }
            else
            {
                return EngineReturnStatus::INVALID_CHANNEL;
            }
        }
    }

    auto& storage = direction == Direction::INPUT ? _audio_in_connections : _audio_out_connections;
    bool realtime = this->realtime();

    AudioConnection con = {.engine_channel = engine_channel,
                           .track_channel = track_channel,
                           .track = track->id()};
    bool added = storage.add(con, !realtime);

    if (added && realtime)
    {
        auto event = direction == Direction::INPUT ? RtEvent::make_add_audio_input_connection_event(con) :
                                                     RtEvent::make_add_audio_output_connection_event(con);
        _send_control_event(event);
        added = _event_receiver.wait_for_response(event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        if (added == false)
        {
            storage.remove(con, false);
            SUSHI_LOG_ERROR("Failed to insert audio connection in realtime thread");
        }
    }
    else if (added == false)
    {
        SUSHI_LOG_ERROR("Max number of {} audio connections reached", direction == Direction::INPUT ? "input" : "output");
        return EngineReturnStatus::ERROR;
    }

    SUSHI_LOG_INFO("Connected engine {} {} to channel {} of track \"{}\"",
                        direction == Direction::INPUT ? "input" : "output", engine_channel, track_channel, track_id);
    return EngineReturnStatus::OK;
}

EngineReturnStatus AudioEngine::_disconnect_audio_channel(int engine_channel,
                                                          int track_channel,
                                                          ObjectId track_id,
                                                          Direction direction)
{
    auto track = _processors.track(track_id);
    if (track == nullptr)
    {
        return EngineReturnStatus::INVALID_TRACK;
    }

    auto& storage = direction == Direction::INPUT ? _audio_in_connections : _audio_out_connections;
    bool realtime = this->realtime();

    AudioConnection con = {.engine_channel = engine_channel, .track_channel = track_channel, .track = track->id()};
    bool removed = storage.remove(con, !realtime);

    if (removed && realtime)
    {
        auto event = direction == Direction::INPUT ? RtEvent::make_remove_audio_input_connection_event(con) :
                                                     RtEvent::make_remove_audio_output_connection_event(con);
        _send_control_event(event);
        removed = _event_receiver.wait_for_response(event.returnable_event()->event_id(), RT_EVENT_TIMEOUT);
        SUSHI_LOG_ERROR_IF(removed == false, "Failed to remove audio connection in realtime thread")
    }
    else if (removed == false)
    {
        SUSHI_LOG_ERROR("Failed to remove {} audio connection", direction == Direction::INPUT ? "input" : "output");
        return EngineReturnStatus::ERROR;
    }

    SUSHI_LOG_INFO("Removed {} audio connection from channel {} of track \"{}\" and engine channel {}",
                         direction == Direction::INPUT ? "input" : "output", track_channel, track->name(), engine_channel);
    return EngineReturnStatus::OK;
}

void AudioEngine::_process_internal_rt_events()
{
    RtEvent event;
    while(_control_queue_in.pop(event))
    {
        switch (event.type())
        {
            case RtEventType::TEMPO:
            case RtEventType::TIME_SIGNATURE:
            case RtEventType::PLAYING_MODE:
            case RtEventType::SYNC_MODE:
            {
                _transport.process_event(event);
                break;
            }
            case RtEventType::INSERT_PROCESSOR:
            {
                auto typed_event = event.processor_operation_event();
                bool inserted = _insert_processor_in_realtime_part(typed_event->instance());
                typed_event->set_handled(inserted);
                break;
            }
            case RtEventType::REMOVE_PROCESSOR:
            {
                auto typed_event = event.processor_reorder_event();
                bool removed = _remove_processor_from_realtime_part(typed_event->processor());
                typed_event->set_handled(removed);
                break;
            }
            case RtEventType::ADD_PROCESSOR_TO_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                auto track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                auto processor = static_cast<Processor*>(_realtime_processors[typed_event->processor()]);
                bool added = false;
                if (track && processor)
                {
                    added = track->add(processor, typed_event->before_processor());
                }
                typed_event->set_handled(added);
                break;
            }
            case RtEventType::REMOVE_PROCESSOR_FROM_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                auto track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                bool removed = false;
                if (track)
                {
                    removed = track->remove(typed_event->processor());
                }
                typed_event->set_handled(removed);
                break;
            }
            case RtEventType::ADD_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                auto track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                typed_event->set_handled(track ? _add_track(track) : false);
                break;
            }
            case RtEventType::REMOVE_TRACK:
            {
                auto typed_event = event.processor_reorder_event();
                auto track = static_cast<Track*>(_realtime_processors[typed_event->track()]);
                typed_event->set_handled(track ? _remove_track(track) : false);
                break;
            }
            case RtEventType::ADD_AUDIO_CONNECTION:
            {
                auto typed_event = event.audio_connection_event();
                assert(_realtime_processors[typed_event->connection().track]);
                auto& storage = typed_event->input_connection() ? _audio_in_connections : _audio_out_connections;
                typed_event->set_handled(storage.add_rt(typed_event->connection()));
                break;
            }
            case RtEventType::REMOVE_AUDIO_CONNECTION:
            {
                auto typed_event = event.audio_connection_event();
                auto& storage = typed_event->input_connection() ? _audio_in_connections : _audio_out_connections;
                typed_event->set_handled(storage.remove_rt(typed_event->connection()));
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
        _retrieve_events_from_output_pipe(output, buffer);
    }
    _retrieve_events_from_output_pipe(_prepost_event_outputs, buffer);
}

void AudioEngine::_retrieve_events_from_output_pipe(RtEventFifo<>& pipe, ControlBuffer& buffer)
{
    while (pipe.empty() == false)
    {
        const RtEvent& event = pipe.pop();
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

void AudioEngine::_copy_audio_to_tracks(ChunkSampleBuffer* input)
{
    for (const auto& c : _audio_in_connections.connections_rt())
    {
        auto engine_in = ChunkSampleBuffer::create_non_owning_buffer(*input, c.engine_channel, 1);
        auto track_in = static_cast<Track*>(_realtime_processors[c.track])->input_channel(c.track_channel);
        track_in = engine_in;
    }
}

void AudioEngine::_copy_audio_from_tracks(ChunkSampleBuffer* output)
{
    output->clear();
    for (const auto& c : _audio_out_connections.connections_rt())
    {
        auto track_out = static_cast<Track*>(_realtime_processors[c.track])->output_channel(c.track_channel);
        auto engine_out = ChunkSampleBuffer::create_non_owning_buffer(*output, c.engine_channel, 1);
        engine_out.add(track_out);
    }
}

void AudioEngine::update_timings()
{
    if (_process_timer.enabled())
    {
        auto engine_timings = _process_timer.timings_for_node(ENGINE_TIMING_ID);
        _event_dispatcher->post_event(new EngineTimingNotificationEvent(*engine_timings, IMMEDIATE_PROCESS));

        _log_timing_print_counter += 1;
        if (_log_timing_print_counter > TIMING_LOG_PRINT_INTERVAL)
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
            if (engine_timings.has_value())
            {
                SUSHI_LOG_INFO("Engine total: avg: {}%, min: {}%, max: {}%",
                               engine_timings->avg_case * 100.0f, engine_timings->min_case * 100.0f, engine_timings->max_case * 100.0f);
            }
            _log_timing_print_counter = 0;
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

bool AudioEngine::_add_track(Track* track)
{
    bool added = false;
    switch (track->type())
    {
        case TrackType::REGULAR:
            added = _audio_graph.add(track);
            break;

        case TrackType::POST:
            if (_post_track == nullptr)
            {
                _post_track = track;
                _post_track->set_event_output(&_prepost_event_outputs);
                added = true;
            }
            break;

        case TrackType::PRE:
            if (_pre_track == nullptr)
            {
                _pre_track = track;
                _pre_track->set_event_output(&_prepost_event_outputs);
                added = true;
            }
    }
    return added;
}

bool AudioEngine::_remove_track(Track* track)
{
    bool removed = false;
    switch (track->type())
    {
        case TrackType::REGULAR:
            removed = _audio_graph.remove(track);
            break;

        case TrackType::POST:
            if (_post_track)
            {
                _post_track = nullptr;
                removed = true;
            }
            break;

        case TrackType::PRE:
            if (_pre_track)
            {
                _pre_track = nullptr;
                removed = true;
            }
    }
    return removed;
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
    for (const auto& r : _cv_in_connections)
    {
        float value = buffer.cv_values[r.cv_id];
        auto ev = RtEvent::make_parameter_change_event(r.processor_id, 0, r.parameter_id, value);
        _send_rt_event(ev);
    }
    // Get gate state changes by xor:ing with previous states
    auto gate_diffs = _prev_gate_values ^ buffer.gate_values;
    if (gate_diffs.any())
    {
        for (const auto& r : _gate_in_connections)
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

} // namespace engine
} // namespace sushi
