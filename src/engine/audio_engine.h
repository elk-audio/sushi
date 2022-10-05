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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ENGINE_H
#define SUSHI_ENGINE_H

#include <vector>
#include <utility>
#include <mutex>

#include "twine/twine.h"

#include "dsp_library/master_limiter.h"
#include "engine/event_dispatcher.h"
#include "engine/base_engine.h"
#include "engine/track.h"
#include "engine/processor_container.h"
#include "engine/receiver.h"
#include "engine/transport.h"
#include "engine/host_control.h"
#include "engine/plugin_library.h"
#include "engine/controller/controller.h"
#include "engine/audio_graph.h"
#include "engine/connection_storage.h"
#include "library/time.h"
#include "library/sample_buffer.h"
#include "library/internal_plugin.h"
#include "library/midi_decoder.h"
#include "library/rt_event_fifo.h"
#include "library/types.h"
#include "library/performance_timer.h"
#include "library/plugin_registry.h"

namespace sushi {
namespace engine {

class ClipDetector
{
public:
    explicit ClipDetector(float sample_rate)
    {
        this->set_sample_rate(sample_rate);
    }

    void set_sample_rate(float samplerate);

    void set_input_channels(int channels);

    void set_output_channels(int channels);
    /**
     * @brief Find clipped samples in a buffer and send notifications
     * @param buffer The audio buffer to process
     * @param queue Endpoint for clipping notifications
     * @param audio_input Set to true if the audio buffer comes directly from the an audio inout (i.e. before any processing)
     */
    void detect_clipped_samples(const ChunkSampleBuffer& buffer, RtSafeRtEventFifo& queue, bool audio_input);

private:

    unsigned int _interval{0};
    std::vector<unsigned int> _input_clip_count;
    std::vector<unsigned int> _output_clip_count;
};

constexpr int MAX_RT_PROCESSOR_ID = 100000;

class AudioEngine : public BaseEngine
{
public:
    SUSHI_DECLARE_NON_COPYABLE(AudioEngine);

    /**
     * @brief Construct a new AudioEngine
     * @param sample_rate The sample to use in Hz
     * @param rt_cpu_cores The maximum number of cpu cores to use for audio processing. Default
     *                     is 1 and means that audio processing is done only in the rt callback
     *                     of the audio frontend.
     *                     With values >1 tracks will be processed in parallel threads.
     * @param debug_mode_sw Enable xenomai specific thread debugging for all audio threads in
     *                      multicore mode.
     * @param event_dispatcher A pointer to a BaseEventDispatcher instance, which AudioEngine takes over ownership of.
     *                         If nullptr, a normal EventDispatcher is created and used.
     */
    explicit AudioEngine(float sample_rate,
                         int rt_cpu_cores = 1,
                         bool debug_mode_sw = false,
                         dispatcher::BaseEventDispatcher* event_dispatcher = nullptr);

     ~AudioEngine() override;

    /**
     * @brief Configure the Engine with a new samplerate.
     * @param sample_rate The new sample rate in Hz
     */
    void set_sample_rate(float sample_rate) override;

    /**
     * @brief Set the number of input audio channels, set by the audio frontend before
     *        starting processing
     * @param channels The number of audio channels to use
     */
    void set_audio_input_channels(int channels) override;

    /**
     * @brief Set the number of output audio channels, set by the audio frontend before
     *        starting processing
     * @param channels The number of audio channels to use
     */
    void set_audio_output_channels(int channels) override;

    /**
     * @brief Set the number of control voltage inputs, set by the audio frontend before
     *        starting processing
     * @param channels The number of cv input channels to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus set_cv_input_channels(int channels) override;

    /**
     * @brief Set the number of control voltage outputs, set by the audio frontend before
     *        starting processing
     * @param channels The number of cv input channels to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus set_cv_output_channels(int channels) override;

    /**
     * @brief Connect an engine input channel to an input channel of a given track.
     *        Not safe to call while the engine is running.
     * @param input_channel Index of the engine input channel to connect.
     * @param track_channel Index of the input channel of the track to connect to.
     * @param track_id The id of the track to connect to.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_input_channel(int input_channel,
                                                   int track_channel,
                                                   ObjectId track_id) override;

    /**
     * @brief Connect an output channel of a track to an engine output channel.
     *        Not safe to call while the engine is running.
     * @param output_channel Index of the engine output channel to connect to.
     * @param track_channel Index of the output channel of the track to connect from.
     * @param track_id The id of the track to connect from.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_output_channel(int output_channel,
                                                    int track_channel,
                                                    ObjectId track_id) override;

    EngineReturnStatus disconnect_audio_input_channel(int engine_channel,
                                                      int track_channel,
                                                      ObjectId track_id) override;

    EngineReturnStatus disconnect_audio_output_channel(int engine_channel,
                                                      int track_channel,
                                                      ObjectId track_id) override;

    std::vector<AudioConnection> audio_input_connections() override;

    std::vector<AudioConnection> audio_output_connections() override;

    /**
     * @brief Connect a stereo pair (bus) from an engine input bus to an input bus of
     *        given track. Not safe to use while the engine in running.
     * @param input_bus The engine input bus to use.
     *        bus 0 refers to channels 0-1, 1 to channels 2-3, etc
     * @param track_bus The input bus of the track to connect to.
     * @param track_id The id of the track to connect to.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_input_bus(int input_bus,
                                               int track_bus,
                                               ObjectId track_id) override;

    /**
     * @brief Connect an output bus of a track to an output bus (stereo pair)
     *        Not safe to use while the engine in running.
     * @param output_bus The engine outpus bus to use.
     *        bus 0 refers to channels 0-1, 1 to channels 2-3, etc
     * @param track_bus The output bus of the track to connect from.
     * @param track_id The id of the track to connect from.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_output_bus(int output_bus,
                                                int track_bus,
                                                ObjectId track_id) override;

    /**
     * @brief Connect a control voltage input to control a parameter on a processor
     * @param processor_name The unique name of the processor.
     * @param parameter_name The name of the parameter to connect to
     * @param cv_input_id The Cv input port id to use
     * @return EngineReturnStatud::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_cv_to_parameter(const std::string& processor_name,
                                               const std::string& parameter_name,
                                               int cv_input_id) override;

    /**
     * @brief Connect a parameter on a processor to a control voltage output
     * @param processor_name The unique name of the processor.
     * @param parameter_name The name of the parameter to connect from
     * @param cv_output_id The Cv output port id to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_cv_from_parameter(const std::string& processor_name,
                                                 const std::string& parameter_name,
                                                 int cv_output_id) override;

    /**
     * @brief Connect a gate input to a processor. Gate changes will be sent as note
     *        on or note off messages to the processor on the selected channel and
     *        with the selected note number.
     * @param processor_name The unique name of the processor.
     * @param gate_input_id The gate input port to use.
     * @param note_no The note number to identify the gate id (0-127).
     * @param channel The channel to use (0-15).
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_to_processor(const std::string& processor_name,
                                                 int gate_input_id,
                                                 int note_no,
                                                 int channel) override;

    /**
     * @brief Connect a processor to a gate output. Note on or note off messages sent
     *        from the processor on the selected channel and note number will be converted
     *        to gate events on the gate output.
     * @param processor_name The unique name of the processor.
     * @param gate_output_id The gate output port to use.
     * @param note_no The note number to identify the gate id (0-127)
     * @param channel The channel to use (0-15).
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_from_processor(const std::string& processor_name,
                                                   int gate_output_id,
                                                   int note_no,
                                                   int channel) override;

    /**
     * @brief Use a selected gate input as sync input.
     * @param gate_input_id The gate input port to use as sync input.
     * @param ppq_ticks Number of ticks per quarternote.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_to_sync(int gate_input_id,
                                            int ppq_ticks) override;

    /**
     * @brief Send sync pulses on a gate output.
     * @param gate_output_id The gate output to use.
     * @param ppq_ticks Number of pulses per quarternote.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_sync_to_gate(int gate_output_id,
                                            int ppq_ticks) override;

    /**
     * @brief Returns whether the engine is running in a realtime mode or not
     * @return true if the engine is currently processing in realtime mode, false otherwise
     */
    bool realtime() override;

    /**
     * @brief Set the engine to operate in realtime mode. In this mode process_chunk and
     *        _send_rt_event is assumed to be called from a realtime thread.
     *        All other function calls are assumed to be made from non-realtime threads
     *
     * @param enabled true to enable realtime mode, false to disable
     */
    void enable_realtime(bool enabled) override;

    /**
     * @brief Process one chunk of audio from in_buffer and write the result to out_buffer
     * @param in_buffer input audio buffer
     * @param out_buffer output audio buffer
     * @param in_controls input control voltage and gate data
     * @param out_controls output control voltage and gate data
     * @param timestamp Current time in microseconds
     * @param sample_count Current number of samples processed
     */
    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                       SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                       ControlBuffer* in_controls,
                       ControlBuffer* out_controls,
                       Time timestamp,
                       int64_t sample_count) override;

    /**
     * @brief Inform the engine of the current system latency
     * @param latency The output latency of the audio system
     */
    void set_output_latency(Time latency) override
    {
        _transport.set_latency(latency);
    }

    /**
     * @brief Set the tempo of the engine. Intended to be called from a non-thread.
     * @param tempo The new tempo in beats (quarter notes) per minute
     */
    void set_tempo(float tempo) override;

    /**
     * @brief Set the time signature of the engine. Intended to be called from a non-thread.
     * @param signature A TimeSignature object describing the new time signature to use
     */
    void set_time_signature(TimeSignature signature) override;

    /**
     * @brief Set the current transport mode, i.e stopped, playing, recording. This will be
     *        passed on to processors. Note stopped here means that audio is still running
     *        but sequencers and similiar should be in a stopped state.
     *        Currently only STOPPED and PLAYING are implemented and default is PLAYING
     * @param mode A TransportMode mode with the current state
     */
    void set_transport_mode(PlayingMode mode) override;

    /**
     * @brief Set the current mode of synchronising the engine tempo and beats. Default is
     *        INTERNAL.
     * @param mode A SyncMode with the current mode of synchronisation
     */
    void set_tempo_sync_mode(SyncMode mode) override;

    /**
     * @brief Set an absolute path to be the base for plugin paths
     *
     * @param path Absolute path of the base plugin folder
     */
    virtual void set_base_plugin_path(const std::string& path) override
    {
        _plugin_library.set_base_plugin_path(path);
    }

    /**
     * @brief Send an RtEvent directly to the realtime thread, should normally only be used
     *        from an rt thread or in a context where the engine is not running in realtime mode
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_rt_event(const RtEvent& event) override;

    /**
     * @brief Create an empty track
     * @param name The unique name of the track to be created.
     * @param channel_count The number of channels in the track.
     * @return the unique id of the track created, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, ObjectId> create_track(const std::string& name, int channel_count) override;

    /**
     * @brief Create an empty track
     * @param name The unique name of the track to be created.
     * @param bus_count The number of stereo pairs in the track.
     * @return EngineInitStatus::OK in case of success, different error code otherwise.
     */
    std::pair<EngineReturnStatus, ObjectId> create_multibus_track(const std::string& name, int bus_count) override;

    std::pair<EngineReturnStatus, ObjectId> create_post_track(const std::string& name) override;

    std::pair<EngineReturnStatus, ObjectId> create_pre_track(const std::string& name) override;

    /**
     * @brief Delete a track, currently assumes that the track is empty before calling
     * @param track_id The unique name of the track to delete
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus delete_track(ObjectId track_id) override;

    /**
     * @brief Create a processor instance, either from internal plugins or loaded from file.
     *        The created plugin can then be added to tracks.
     * @param plugin_info
     * @param processor_name
     * @return
     */
    std::pair <EngineReturnStatus, ObjectId> create_processor(const PluginInfo& plugin_info,
                                                              const std::string& processor_name) override;

    /**
     * @brief Add a plugin to a track. The plugin must not currently be active on any track.
     * @param track_id The id of the track to add the plugin to.
     * @param plugin_id The id of the plugin to add.
     * @param before_plugin_id If this parameter is passed with a value, the plugin will be
     *        inserted after this plugin. If this parameter is empty, the plugin will be
     *        placed at the back of the track's processing chain.
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus add_plugin_to_track(ObjectId plugin_id,
                                           ObjectId track_id,
                                           std::optional<ObjectId> before_plugin_id = std::nullopt) override;

    /**
     * @brief Remove a given plugin from a track and delete it
     * @param track_id The id of the track that contains the plugin
     * @param plugin_id The id of the plugin
     * @return EngineReturnStatus::OK in case of success, different error code otherwise
     */
    EngineReturnStatus remove_plugin_from_track(ObjectId plugin_id, ObjectId track_id) override;

    /**
     * @brief Delete and unload a plugin instance from Sushi. The plugin must
     *        not currently be active on any track.
     * @param plugin_id The id of the plugin to delete.
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus delete_plugin(ObjectId plugin_id) override;

    /**
     * @brief Enable audio clip detection on engine inputs
     * @param enabled Enable if true, disable if false
     */
    void enable_input_clip_detection(bool enabled) override
    {
        _input_clip_detection_enabled = enabled;
    }

    /**
    * @brief Return the state of clip detection on audio inputs
    * @param true if clip detection is enabled, false if disabled.
    */
    bool input_clip_detection() const override
    {
        return _input_clip_detection_enabled;
    }

    /**
     * @brief Enable audio clip detection on engine outputs
     * @param enabled Enable if true, disable if false
     */
    void enable_output_clip_detection(bool enabled) override
    {
        _output_clip_detection_enabled = enabled;
    }

    /**
    * @brief Return the state of clip detection on outputs
    * @param true if clip detection is enabled, false if disabled.
    */
    bool output_clip_detection() const override
    {
        return _output_clip_detection_enabled;
    }

    /**
     * @brief Enable master limiter on outputs
     * @param enabled Enabled if true, disable if false
     */
    void enable_master_limiter(bool enabled) override
    {
        _master_limiter_enabled = enabled;
    }

    /**
     * @brief Return the state of master limiter on outputs
     * @param true if master limiter is enabled, false if disabled.
     */
    bool master_limiter() const override
    {
        return _master_limiter_enabled;
    }

    sushi::dispatcher::BaseEventDispatcher* event_dispatcher() override
    {
        return _event_dispatcher.get();
    }

    sushi::engine::Transport* transport() override
    {
        return &_transport;
    }

    performance::BasePerformanceTimer* performance_timer() override
    {
        return &_process_timer;
    }

    const BaseProcessorContainer* processor_container() override
    {
        return &_processors;
    }

    /**
     * @brief Print the current processor timings (in enabled) in the log
     */
    void update_timings() override;

private:
    enum class Direction : bool
    {
        INPUT = true,
        OUTPUT = false
    };
    /**
     * @brief Register a newly created processor in all lookup containers
     *        and take ownership of it.
     * @param processor Processor to register
     */
    EngineReturnStatus _register_processor(std::shared_ptr<Processor> processor, const std::string& name);

    /**
     * @brief Remove a processor from the engine. The processor must not be active
     *        on any track when called. The engine does not hold any references
     *        to the processor after this function has returned.
     * @param processor A pointer to the instance of the processor to delete
     */
    void _deregister_processor(Processor* processor);

    /**
     * @brief Add a registered processor to the realtime processing part.
     *        Must be called before a processor can be used to process audio.
     * @param processor Processor to insert
     */
    bool _insert_processor_in_realtime_part(Processor* processor);

    /**
     * @brief Remove a processor from the realtime processing part
     *        Must be called before de-registering a processor.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    bool _remove_processor_from_realtime_part(ObjectId processor);

    /**
     * @brief Remove all audio connections from track
     * @param track_id The id of the track to remove from
     */
    void _remove_connections_from_track(ObjectId track_id);

    /**
     * @brief Register a newly created track
     * @param track Pointer to the track
     * @return OK if successful, error code otherwise
     */
    EngineReturnStatus _register_new_track(const std::string& name, std::shared_ptr<Track> track);

    /**
    * @brief Called from a non-realtime thread to process a control event in the realtime thread
    * @param event The event to process
    * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
    */

    std::pair<EngineReturnStatus, ObjectId> _create_master_track(const std::string& name, TrackType type, int channels);

    EngineReturnStatus _send_control_event(RtEvent& event);

    EngineReturnStatus _connect_audio_channel(int engine_channel, int track_channel, ObjectId track_id, Direction direction);

    EngineReturnStatus _disconnect_audio_channel(int engine_channel, int track_channel, ObjectId track_id, Direction direction);

    void _process_internal_rt_events();

    void _send_rt_events_to_processors();

    void _send_rt_event(const RtEvent& event);

    inline void _retrieve_events_from_tracks(ControlBuffer& buffer);

    inline void _retrieve_events_from_output_pipe(RtEventFifo<>& pipe, ControlBuffer& buffer);

    inline void _copy_audio_to_tracks(ChunkSampleBuffer* input);

    inline void _copy_audio_from_tracks(ChunkSampleBuffer* output);

    /**
     * @brief Add a track to the audio engine, if engine is running, this must be called from the
     *        rt thread before/after processing. If not running, then this function can safely be
     *        called from a non-rt thread
     * @param track The track to add
     * @return True if successful, false otherwise
     */
    bool _add_track(Track* track);

    /**
     * @brief Remove a track from the audio engine, if engine is running, this must be called from
     *        the rt thread before/after processing. If not running, then this function can safely be
     *        called from a non-rt thread
     * @param track The track to remove.
     * @return True if successful, false otherwise
 */
    bool _remove_track(Track* track);

    void print_timings_to_file(const std::string& filename);

    void _route_cv_gate_ins(ControlBuffer& buffer);

    PluginRegistry _plugin_registry;
    ProcessorContainer _processors;

    // Processors in the realtime part indexed by their unique 32 bit id
    // Only to be accessed from the process callback in rt mode.
    std::vector<Processor*>    _realtime_processors{MAX_RT_PROCESSOR_ID, nullptr};
    AudioGraph                 _audio_graph;

    Track* _pre_track{nullptr};
    Track* _post_track{nullptr};
    ChunkSampleBuffer _input_swap_buffer;
    ChunkSampleBuffer _output_swap_buffer;

    ConnectionStorage<AudioConnection> _audio_in_connections;
    ConnectionStorage<AudioConnection> _audio_out_connections;
    std::vector<CvConnection>    _cv_in_connections;
    std::vector<GateConnection>  _gate_in_connections;

    BitSet32 _prev_gate_values{0};
    BitSet32 _outgoing_gate_values{0};

    std::atomic<RealtimeState> _state{RealtimeState::STOPPED};

    RtSafeRtEventFifo _control_queue_in;
    RtSafeRtEventFifo _main_in_queue;
    RtSafeRtEventFifo _main_out_queue;
    RtSafeRtEventFifo _control_queue_out;
    std::mutex _in_queue_lock;
    RtEventFifo<> _prepost_event_outputs;
    receiver::AsynchronousEventReceiver _event_receiver{&_control_queue_out};
    Transport _transport;
    PluginLibrary _plugin_library;

    std::unique_ptr<dispatcher::BaseEventDispatcher> _event_dispatcher;
    HostControl _host_control{nullptr, &_transport, &_plugin_library};

    performance::PerformanceTimer _process_timer;
    int  _log_timing_print_counter{0};

    bool _input_clip_detection_enabled{false};
    bool _output_clip_detection_enabled{false};
    ClipDetector _clip_detector;

    bool _master_limiter_enabled{false};
    std::vector<dsp::MasterLimiter<AUDIO_CHUNK_SIZE>> _master_limiters;
};

/**
 * @brief Helper function to encapsulate state changes from transient states
 * @param current_state The current state of the engine
 * @return A new, non-transient state
 */
RealtimeState update_state(RealtimeState current_state);

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
