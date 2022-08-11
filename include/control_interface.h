/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Abstract interface for external control of sushi over rpc, osc or similar
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONTROL_INTERFACE_H
#define SUSHI_CONTROL_INTERFACE_H

#include <utility>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <chrono>

namespace sushi {
namespace ext {

using Time = std::chrono::microseconds;

enum class ControlStatus
{
    OK,
    ERROR,
    UNSUPPORTED_OPERATION,
    NOT_FOUND,
    OUT_OF_RANGE,
    INVALID_ARGUMENTS
};

enum class PlayingMode
{
    STOPPED,
    PLAYING,
    RECORDING
};

enum class SyncMode
{
    INTERNAL,
    MIDI,
    GATE,
    LINK
};

struct TimeSignature
{
    int numerator;
    int denominator;
};

struct CpuTimings
{
    float avg;
    float min;
    float max;
};

enum class PluginType
{
    INTERNAL,
    VST2X,
    VST3X,
    LV2
};

enum class ParameterType
{
    BOOL,
    INT,
    FLOAT
};

struct ParameterInfo
{
    int             id;
    ParameterType   type;
    std::string     label;
    std::string     name;
    std::string     unit;
    bool            automatable;
    float           min_domain_value;
    float           max_domain_value;
};

struct PropertyInfo
{
    int         id;
    std::string name;
    std::string label;
};

struct ProcessorInfo
{
    int         id;
    std::string label;
    std::string name;
    int         parameter_count;
    int         program_count;
};

struct ProgramInfo
{
    int         id;
    std::string name;
};

enum class TrackType
{
    REGULAR,
    PRE,
    POST
};

struct TrackInfo
{
    int         id;
    std::string label;
    std::string name;
    int         channels;
    int         buses;
    TrackType   type;
    std::vector<int> processors;
};

struct ProcessorState
{
    std::optional<bool> bypassed;
    std::optional<int>  program;
    std::vector<std::pair<int, float>> parameters;
    std::vector<std::pair<int, std::string>> properties;
    std::vector<std::byte> binary_data;
};

struct SushiBuildInfo
{
    std::string                 version;
    std::vector<std::string>    build_options;
    int                         audio_buffer_size;
    std::string                 commit_hash;
    std::string                 build_date;
};

enum class MidiChannel
{
    MIDI_CH_1,
    MIDI_CH_2,
    MIDI_CH_3,
    MIDI_CH_4,
    MIDI_CH_5,
    MIDI_CH_6,
    MIDI_CH_7,
    MIDI_CH_8,
    MIDI_CH_9,
    MIDI_CH_10,
    MIDI_CH_11,
    MIDI_CH_12,
    MIDI_CH_13,
    MIDI_CH_14,
    MIDI_CH_15,
    MIDI_CH_16,
    MIDI_CH_OMNI
};

struct AudioConnection
{
    int track_id;
    int track_channel;
    int engine_channel;
};

struct CvConnection
{
    int track_id;
    int parameter_id;
    int cv_port_id;
};

struct GateConnection
{
    int processor_id;
    int gate_port_id;
    int channel;
    int note_no;
};

struct MidiKbdConnection
{
    int         track_id;
    MidiChannel channel;
    int         port;
    bool        raw_midi;
};

struct MidiCCConnection
{
    int         processor_id;
    int         parameter_id;
    MidiChannel channel;
    int         port;
    int         cc_number;
    int         min_range;
    int         max_range;
    bool        relative_mode;
};

struct MidiPCConnection
{
    int         processor_id;
    MidiChannel channel;
    int         port;
};

enum class NotificationType
{
    TRANSPORT_UPDATE,
    CPU_TIMING_UPDATE,
    TRACK_UPDATE,
    PROCESSOR_UPDATE,
    PARAMETER_CHANGE,
    PROPERTY_CHANGE
};

enum class ProcessorAction
{
    ADDED,
    DELETED
};

enum class TrackAction
{
    ADDED,
    DELETED
};

enum class TransportAction
{
    PLAYING_MODE_CHANGED,
    SYNC_MODE_CHANGED,
    TIME_SIGNATURE_CHANGED,
    TEMPO_CHANGED
};

struct MidiKbdConnectionState
{
    std::string track;
    MidiChannel channel;
    int         port;
    bool        raw_midi;
};

struct MidiCCConnectionState
{
    std::string processor;
    int         parameter_id;
    MidiChannel channel;
    int         port;
    int         cc_number;
    float       min_range;
    float       max_range;
    bool        relative_mode;
};

struct MidiPCConnectionState
{
    std::string processor;
    MidiChannel channel;
    int         port;
};

struct MidiState
{
    int inputs;
    int outputs;
    std::vector<MidiKbdConnectionState> kbd_input_connections;
    std::vector<MidiKbdConnectionState> kbd_output_connections;
    std::vector<MidiCCConnectionState> cc_connections;
    std::vector<MidiPCConnectionState> pc_connections;
    std::vector<int> enabled_clock_outputs;
};

struct OscParameterState
{
    std::string processor;
    std::vector<int> parameter_ids;
};

struct OscState
{
    bool enable_all_processor_outputs;
    std::vector<OscParameterState> enabled_processor_outputs;
};

struct TrackAudioConnectionState
{
    std::string  track;
    int          track_channel;
    int          engine_channel;
};

struct EngineState
{
    float           sample_rate;
    float           tempo;
    PlayingMode     playing_mode;
    SyncMode        sync_mode;
    TimeSignature   time_signature;
    bool            input_clip_detection;
    bool            output_clip_detection;
    bool            master_limiter;
    int             used_audio_inputs;
    int             used_audio_outputs;
    std::vector<TrackAudioConnectionState> input_connections;
    std::vector<TrackAudioConnectionState> output_connections;
};

struct PluginClass
{
    std::string     name;
    std::string     label;
    std::string     uid;
    std::string     path;
    PluginType      type;
    ProcessorState  state;
};

struct TrackState
{
    std::string     name;
    std::string     label;
    int             channels;
    int             buses;
    TrackType       type;
    ProcessorState  track_state;
    std::vector<PluginClass>    processors;
};

struct SessionState
{
    SushiBuildInfo      sushi_info;
    std::string         save_date;
    OscState            osc_state;
    MidiState           midi_state;
    EngineState         engine_state;
    std::vector<TrackState> tracks;
};

class SystemController
{
public:
    virtual ~SystemController() = default;

    virtual std::string         get_sushi_version() const = 0;
    virtual SushiBuildInfo      get_sushi_build_info() const = 0;
    virtual int                 get_input_audio_channel_count() const = 0;
    virtual int                 get_output_audio_channel_count() const = 0;

protected:
    SystemController() = default;
};

class TransportController
{
public:
    virtual ~TransportController() = default;

    virtual float               get_samplerate() const = 0;
    virtual PlayingMode         get_playing_mode() const = 0;
    virtual SyncMode            get_sync_mode() const = 0;
    virtual TimeSignature       get_time_signature() const = 0;
    virtual float               get_tempo() const = 0;

    virtual void                set_sync_mode(SyncMode sync_mode) = 0;
    virtual void                set_playing_mode(PlayingMode playing_mode) = 0;
    virtual ControlStatus       set_tempo(float tempo) = 0;
    virtual ControlStatus       set_time_signature(TimeSignature signature) = 0;

protected:
    TransportController() = default;
};

class TimingController
{
public:
    virtual ~TimingController() = default;

    virtual bool                                    get_timing_statistics_enabled() const = 0;
    virtual void                                    set_timing_statistics_enabled(bool enabled) = 0;

    virtual std::pair<ControlStatus, CpuTimings>    get_engine_timings() const = 0;
    virtual std::pair<ControlStatus, CpuTimings>    get_track_timings(int track_id) const = 0;
    virtual std::pair<ControlStatus, CpuTimings>    get_processor_timings(int processor_id) const = 0;
    virtual ControlStatus                           reset_all_timings() = 0;
    virtual ControlStatus                           reset_track_timings(int track_id) = 0;
    virtual ControlStatus                           reset_processor_timings(int processor_id) = 0;

protected:
    TimingController() = default;
};

class KeyboardController
{
public:
    virtual ~KeyboardController() = default;

    virtual ControlStatus send_note_on(int track_id, int channel, int note, float velocity) = 0;
    virtual ControlStatus send_note_off(int track_id, int channel, int note, float velocity) = 0;
    virtual ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) = 0;
    virtual ControlStatus send_aftertouch(int track_id, int channel, float value) = 0;
    virtual ControlStatus send_pitch_bend(int track_id, int channel, float value) = 0;
    virtual ControlStatus send_modulation(int track_id, int channel, float value) = 0;

protected:
    KeyboardController() = default;
};

class AudioGraphController
{
public:
    virtual ~AudioGraphController() = default;

    virtual std::vector<ProcessorInfo>                            get_all_processors() const = 0;
    virtual std::vector<TrackInfo>                                get_all_tracks() const = 0;
    virtual std::pair<ControlStatus, int>                         get_track_id(const std::string& track_name) const = 0;
    virtual std::pair<ControlStatus, TrackInfo>                   get_track_info(int track_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<ProcessorInfo>>  get_track_processors(int track_id) const = 0;
    virtual std::pair<ControlStatus, int>                         get_processor_id(const std::string& processor_name) const = 0;
    virtual std::pair<ControlStatus, ProcessorInfo>               get_processor_info(int processor_id) const = 0;
    virtual std::pair<ControlStatus, bool>                        get_processor_bypass_state(int processor_id) const = 0;
    virtual std::pair<ControlStatus, ProcessorState>              get_processor_state(int processor_id) const = 0;

    virtual ControlStatus set_processor_bypass_state(int processor_id, bool bypass_enabled) = 0;
    virtual ControlStatus set_processor_state(int processor_id, const ProcessorState& state) = 0;

    virtual ControlStatus create_track(const std::string& name, int channels) = 0;
    virtual ControlStatus create_multibus_track(const std::string& name, int buses) = 0;
    virtual ControlStatus create_pre_track(const std::string& name) = 0;
    virtual ControlStatus create_post_track(const std::string& name) = 0;
    virtual ControlStatus move_processor_on_track(int processor_id, int source_track_id, int dest_track_id, std::optional<int> before_processor_id) = 0;
    virtual ControlStatus create_processor_on_track(const std::string& name, const std::string& uid, const std::string& file,
                                                      PluginType type, int track_id, std::optional<int> before_processor_id) = 0;

    virtual ControlStatus delete_processor_from_track(int processor_id, int track_id) = 0;
    virtual ControlStatus delete_track(int track_id) = 0;

protected:
    AudioGraphController() = default;
};

class ProgramController
{
public:
    virtual ~ProgramController() = default;

    virtual std::pair<ControlStatus, int>                       get_processor_current_program(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::string>               get_processor_current_program_name(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::string>               get_processor_program_name(int processor_id, int program_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<std::string>>  get_processor_programs(int processor_id) const = 0;

    virtual ControlStatus                                       set_processor_program(int processor_id, int program_id)= 0;

protected:
    ProgramController() = default;
};

class ParameterController
{
public:
    virtual ~ParameterController() = default;

    virtual std::pair<ControlStatus, std::vector<ParameterInfo>>  get_processor_parameters(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>>  get_track_parameters(int processor_id) const = 0;
    virtual std::pair<ControlStatus, int>                         get_parameter_id(int processor_id, const std::string& parameter) const = 0;
    virtual std::pair<ControlStatus, ParameterInfo>               get_parameter_info(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, float>                       get_parameter_value(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, float>                       get_parameter_value_in_domain(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, std::string>                 get_parameter_value_as_string(int processor_id, int parameter_id) const = 0;
    virtual ControlStatus                                         set_parameter_value(int processor_id, int parameter_id, float value) = 0;

    virtual std::pair<ControlStatus, std::vector<PropertyInfo>>   get_processor_properties(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<PropertyInfo>>   get_track_properties(int processor_id) const = 0;
    virtual std::pair<ControlStatus, int>                         get_property_id(int processor_id, const std::string& parameter) const = 0;
    virtual std::pair<ControlStatus, PropertyInfo>                get_property_info(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, std::string>                 get_property_value(int processor_id, int parameter_id) const = 0;
    virtual ControlStatus                                         set_property_value(int processor_id, int parameter_id, const std::string& value) = 0;

protected:
    ParameterController() = default;
};

class MidiController
{
public:
    virtual ~MidiController() = default;

    virtual int                            get_input_ports() const = 0;
    virtual int                            get_output_ports() const = 0;
    virtual std::vector<MidiKbdConnection> get_all_kbd_input_connections() const = 0;
    virtual std::vector<MidiKbdConnection> get_all_kbd_output_connections() const = 0;
    virtual std::vector<MidiCCConnection>  get_all_cc_input_connections() const = 0;
    virtual std::vector<MidiPCConnection>  get_all_pc_input_connections() const = 0;
    virtual std::pair<ControlStatus, std::vector<MidiCCConnection>> get_cc_input_connections_for_processor(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<MidiPCConnection>> get_pc_input_connections_for_processor(int processor_id) const = 0;

    virtual bool                           get_midi_clock_output_enabled(int port) const = 0;
    virtual ControlStatus                  set_midi_clock_output_enabled(bool enabled, int port) = 0;

    virtual ControlStatus connect_kbd_input_to_track(int track_id, MidiChannel channel, int port, bool raw_midi) = 0;
    virtual ControlStatus connect_kbd_output_from_track(int track_id, MidiChannel channel, int port) = 0;
    virtual ControlStatus connect_cc_to_parameter(int processor_id, int parameter_id, MidiChannel channel, int port,
                                                  int cc_number, float min_range, float max_range, bool relative_mode) = 0;
    virtual ControlStatus connect_pc_to_processor(int processor_id, MidiChannel channel, int port) = 0;

    virtual ControlStatus disconnect_kbd_input(int track_id, MidiChannel channel, int port, bool raw_midi) = 0;
    virtual ControlStatus disconnect_kbd_output(int track_id, MidiChannel channel, int port) = 0;
    virtual ControlStatus disconnect_cc(int processor_id, MidiChannel channel, int port, int cc_number) = 0;
    virtual ControlStatus disconnect_pc(int processor_id, MidiChannel channel, int port) = 0;
    virtual ControlStatus disconnect_all_cc_from_processor(int processor_id) = 0;
    virtual ControlStatus disconnect_all_pc_from_processor(int processor_id) = 0;

protected:
    MidiController() = default;
};

class AudioRoutingController
{
public:
    virtual ~AudioRoutingController() = default;

    virtual std::vector<AudioConnection> get_all_input_connections() const = 0;
    virtual std::vector<AudioConnection> get_all_output_connections() const = 0;
    virtual std::vector<AudioConnection> get_input_connections_for_track(int track_id) const = 0;
    virtual std::vector<AudioConnection> get_output_connections_for_track(int track_id) const = 0;

    virtual ControlStatus                connect_input_channel_to_track(int track_id, int track_channel, int input_channel) = 0;
    virtual ControlStatus                connect_output_channel_to_track(int track_id, int track_channel, int output_channel) = 0;

    virtual ControlStatus                disconnect_input(int track_id, int track_channel, int input_channel) = 0;
    virtual ControlStatus                disconnect_output(int track_id, int track_channel, int output_channel) = 0;
    virtual ControlStatus                disconnect_all_inputs_from_track(int track_id) = 0;
    virtual ControlStatus                disconnect_all_outputs_from_track(int track_id) = 0;

protected:
    AudioRoutingController() = default;
};

class CvGateController
{
public:
    virtual ~CvGateController() = default;

    virtual int                         get_cv_input_ports() const = 0;
    virtual int                         get_cv_output_ports() const = 0;


    virtual std::vector<CvConnection>   get_all_cv_input_connections() const = 0;
    virtual std::vector<CvConnection>   get_all_cv_output_connections() const = 0;
    virtual std::vector<GateConnection> get_all_gate_input_connections() const = 0;
    virtual std::vector<GateConnection> get_all_gate_output_connections() const = 0;
    virtual std::pair<ControlStatus, std::vector<CvConnection>>   get_cv_input_connections_for_processor(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<CvConnection>>   get_cv_output_connections_for_processor(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<GateConnection>> get_gate_input_connections_for_processor(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<GateConnection>> get_gate_output_connections_for_processor(int processor_id) const = 0;

    virtual ControlStatus               connect_cv_input_to_parameter(int processor_id, int parameter_id, int cv_input_id) = 0;
    virtual ControlStatus               connect_cv_output_from_parameter(int processor_id, int parameter_id, int cv_output_id) = 0;
    virtual ControlStatus               connect_gate_input_to_processor(int processor_id, int gate_input_id, int channel, int note_no) = 0;
    virtual ControlStatus               connect_gate_output_from_processor(int processor_id, int gate_output_id, int channel, int note_no) = 0;

    virtual ControlStatus               disconnect_cv_input(int processor_id, int parameter_id, int cv_input_id) = 0;
    virtual ControlStatus               disconnect_cv_output(int processor_id, int parameter_id, int cv_output_id) = 0;
    virtual ControlStatus               disconnect_gate_input(int processor_id, int gate_input_id, int channel, int note_no) = 0;
    virtual ControlStatus               disconnect_gate_output(int processor_id, int gate_output_id, int channel, int note_no) = 0;
    virtual ControlStatus               disconnect_all_cv_inputs_from_processor(int processor_id) = 0;
    virtual ControlStatus               disconnect_all_cv_outputs_from_processor(int processor_id) = 0;
    virtual ControlStatus               disconnect_all_gate_inputs_from_processor(int processor_id) = 0;
    virtual ControlStatus               disconnect_all_gate_outputs_from_processor(int processor_id) = 0;

protected:
    CvGateController() = default;
};

class OscController
{
public:
    virtual ~OscController() = default;

    virtual std::string get_send_ip() const = 0;
    virtual int get_send_port() const = 0;
    virtual int get_receive_port() const = 0;
    virtual std::vector<std::string> get_enabled_parameter_outputs() const = 0;
    virtual ControlStatus enable_output_for_parameter(int processor_id, int parameter_id) = 0;
    virtual ControlStatus disable_output_for_parameter(int processor_id, int parameter_id) = 0;
    virtual ControlStatus enable_all_output() = 0;
    virtual ControlStatus disable_all_output() = 0;

protected:
    OscController() = default;
};

class SessionController
{
public:
    virtual ~SessionController() = default;

    virtual SessionState save_session() const = 0;
    virtual ControlStatus restore_session(const SessionState& state) = 0;
};

class ControlNotification
{
public:
    virtual ~ControlNotification() = default;

    NotificationType type() const {return _type;}
    Time timestamp() const {return _timestamp;}

protected:
    ControlNotification(NotificationType type, Time timestamp) : _type(type),
                                                                 _timestamp(timestamp) {}

private:
    NotificationType _type;
    Time _timestamp;
};

class ControlListener
{
public:
    virtual void notification(const ControlNotification* notification) = 0;
};

class SushiControl
{
public:
    virtual ~SushiControl() = default;

    SystemController*       system_controller() {return _system_controller;}
    TransportController*    transport_controller() {return _transport_controller;}
    TimingController*       timing_controller() {return _timing_controller;}
    KeyboardController*     keyboard_controller() {return _keyboard_controller;}
    AudioGraphController*   audio_graph_controller() {return _audio_graph_controller;}
    ProgramController*      program_controller() {return _program_controller;}
    ParameterController*    parameter_controller() {return _parameter_controller;}
    MidiController*         midi_controller() {return _midi_controller;}
    AudioRoutingController* audio_routing_controller() {return _audio_routing_controller;}
    CvGateController*       cv_gate_controller() {return _cv_gate_controller;}
    OscController*          osc_controller() {return _osc_controller;}
    SessionController*      session_controller() {return _session_controller;}

    virtual ControlStatus   subscribe_to_notifications(NotificationType type, ControlListener* listener) = 0;

protected:
    SushiControl(SystemController*       system_controller,
                 TransportController*    transport_controller,
                 TimingController*       timing_controller,
                 KeyboardController*     keyboard_controller,
                 AudioGraphController*   audio_graph_controller,
                 ProgramController*      program_controller,
                 ParameterController*    parameter_controller,
                 MidiController*         midi_controller,
                 AudioRoutingController* audio_routing_controller,
                 CvGateController*       cv_gate_controller,
                 OscController*          osc_controller,
                 SessionController*      session_controller) : _system_controller(system_controller),
                                                               _transport_controller(transport_controller),
                                                               _timing_controller(timing_controller),
                                                               _keyboard_controller(keyboard_controller),
                                                               _audio_graph_controller(audio_graph_controller),
                                                               _program_controller(program_controller),
                                                               _parameter_controller(parameter_controller),
                                                               _midi_controller(midi_controller),
                                                               _audio_routing_controller(audio_routing_controller),
                                                               _cv_gate_controller(cv_gate_controller),
                                                               _osc_controller(osc_controller),
                                                               _session_controller(session_controller){}

private:
    SystemController*           _system_controller;
    TransportController*        _transport_controller;
    TimingController*           _timing_controller;
    KeyboardController*         _keyboard_controller;
    AudioGraphController*       _audio_graph_controller;
    ProgramController*          _program_controller;
    ParameterController*        _parameter_controller;
    MidiController*             _midi_controller;
    AudioRoutingController*     _audio_routing_controller;
    CvGateController*           _cv_gate_controller;
    OscController*              _osc_controller;
    SessionController*          _session_controller;
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_INTERFACE_H
