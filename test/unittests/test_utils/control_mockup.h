#ifndef SUSHI_CONTROL_MOCKUP_H
#define SUSHI_CONTROL_MOCKUP_H

#ifndef __clang__
#include <bits/stdc++.h>
#endif

#include <unordered_map>

#include "control_interface.h"

namespace sushi {
namespace ext {

const ParameterInfo parameter_1{0, ParameterType::INT, "param 1", "param 1", "unit", false, 0, 0};
const ParameterInfo parameter_2{1, ParameterType::FLOAT, "param 2", "param 2", "unit", true, 1, 1};
const ParameterInfo parameter_3{2, ParameterType::BOOL, "param 3", "param 3", "unit", false, -1, -1};
const PropertyInfo property_1{1, "property_1", "Property 1"};
const std::vector<ParameterInfo> parameters{parameter_1, parameter_2, parameter_3};

const ProcessorInfo processor_1{0, "proc 1", "proc 1", 0 , 0};
const ProcessorInfo processor_2{1, "proc 2", "proc 2", 1 , 1};
const std::vector<ProcessorInfo> processors{processor_1, processor_2};

const TrackInfo track1{0, "track 1", "track 1", 0, 0, TrackType::REGULAR, {}};
const TrackInfo track2{1, "track 2", "track 2", 1, 1, TrackType::REGULAR, {}};
const std::vector<TrackInfo> tracks{track1, track2};

constexpr float                 DEFAULT_SAMPLERATE = 48000.0f;
constexpr float                 DEFAULT_TEMPO = 120.0f;
constexpr float                 DEFAULT_PARAMETER_VALUE = 0.745f;
constexpr auto                  DEFAULT_STRING_PROPERTY = "string property";
constexpr bool                  DEFAULT_TIMING_STATISTICS_ENABLED = false;
constexpr bool                  DEFAULT_BYPASS_STATE = false;
constexpr PlayingMode           DEFAULT_PLAYING_MODE = PlayingMode::PLAYING;
constexpr SyncMode              DEFAULT_SYNC_MODE = SyncMode::INTERNAL;
constexpr TimeSignature         DEFAULT_TIME_SIGNATURE = TimeSignature{4, 4};
constexpr ControlStatus         DEFAULT_CONTROL_STATUS = ControlStatus::OK;
constexpr CpuTimings            DEFAULT_TIMINGS = CpuTimings{1.0f, 0.5f, 1.5f};
constexpr int                   DEFAULT_PROGRAM_ID = 1;
constexpr auto                  DEFAULT_PROGRAM_NAME = "program 1";
const std::vector<std::string>  DEFAULT_PROGRAMS = {DEFAULT_PROGRAM_NAME, "program 2"};


class TestableController
{
public:
    std::unordered_map<std::string, std::string> get_args_from_last_call()
    {
        return _args_from_last_call;
    }

    bool was_recently_called()
    {
        return _recently_called;
    }

    void clear_recent_call()
    {
        _recently_called = false;
    }

    void force_return_status(ControlStatus status)
    {
        _return_status = status;
    }

protected:
    TestableController() = default;

    std::unordered_map<std::string,std::string> _args_from_last_call;
    ControlStatus _return_status{DEFAULT_CONTROL_STATUS};
    bool _recently_called{false};
};

class SystemControllerMockup : public SystemController, public TestableController
{
public:
    std::string get_sushi_version() const override {return "";}

    SushiBuildInfo get_sushi_build_info() const override {return SushiBuildInfo();}

    int get_input_audio_channel_count() const override {return 0;}

    int get_output_audio_channel_count() const override {return 0;}
};

class TransportControllerMockup : public TransportController, public TestableController
{
public:
    float get_samplerate() const override {return DEFAULT_SAMPLERATE;}

    PlayingMode get_playing_mode() const override {return DEFAULT_PLAYING_MODE;}

    SyncMode get_sync_mode() const override {return DEFAULT_SYNC_MODE;}

    TimeSignature get_time_signature() const override {return DEFAULT_TIME_SIGNATURE;}

    float get_tempo() const override {return DEFAULT_TEMPO;}

    void set_sync_mode(SyncMode sync_mode) override
    {
        _args_from_last_call.clear();
        switch (sync_mode)
        {
            case ext::SyncMode::GATE:       _args_from_last_call["sync mode"] = "GATE"; break;
            case ext::SyncMode::INTERNAL:   _args_from_last_call["sync mode"] = "INTERNAL"; break;
            case ext::SyncMode::LINK:       _args_from_last_call["sync mode"] = "LINK"; break;
            case ext::SyncMode::MIDI:       _args_from_last_call["sync mode"] = "MIDI"; break;
            default:                        _args_from_last_call["sync mode"] = "UNKNOWN MODE";
        }

        _recently_called = true;
    };

    void set_playing_mode(PlayingMode playing_mode) override
    {
        _args_from_last_call.clear();
        switch (playing_mode)
        {
            case ext::PlayingMode::STOPPED:         _args_from_last_call["playing mode"] = "STOPPED"; break;
            case ext::PlayingMode::RECORDING:       _args_from_last_call["playing mode"] = "RECORDING"; break;
            case ext::PlayingMode::PLAYING:         _args_from_last_call["playing mode"] = "PLAYING"; break;
            default:                                _args_from_last_call["playing mode"] = "UNKWON MODE";
        }
        _recently_called = true;
    };

    ControlStatus set_tempo(float tempo) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["tempo"] = std::to_string(tempo);
        _recently_called = true;
        return DEFAULT_CONTROL_STATUS;
    };

    ControlStatus set_time_signature(TimeSignature signature) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["numerator"] = std::to_string(signature.numerator);
        _args_from_last_call["denominator"] = std::to_string(signature.denominator);
        _recently_called = true;
        return DEFAULT_CONTROL_STATUS;
    };
};

class TimingControllerMockup : public TimingController, public TestableController
{
public:
    bool get_timing_statistics_enabled() const override
    {
        return DEFAULT_TIMING_STATISTICS_ENABLED;
    }

    void set_timing_statistics_enabled(bool enabled) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["enabled"] = std::to_string(enabled);
        _recently_called = true;
    };

    std::pair<ControlStatus, CpuTimings> get_engine_timings() const override
    {
        return {_return_status, DEFAULT_TIMINGS};
    }

    std::pair<ControlStatus, CpuTimings> get_track_timings(int /*track_id*/) const override
    {
        return {_return_status, DEFAULT_TIMINGS};
    }

    std::pair<ControlStatus, CpuTimings> get_processor_timings(int /*processor_id*/) const override
    {
        return {_return_status, DEFAULT_TIMINGS};
    }

    ControlStatus reset_all_timings() override
    {
        _recently_called = true;
        return _return_status;
    }

    ControlStatus reset_track_timings(int track_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus reset_processor_timings(int processor_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor_id"] = std::to_string(processor_id);
        _recently_called = true;
        return _return_status;
    }

};

class KeyboardControllerMockup : public KeyboardController, public TestableController
{
public:
    ControlStatus send_note_on(int track_id, int channel, int note, float velocity) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["velocity"] = std::to_string(velocity);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus send_note_off(int track_id, int channel, int note, float velocity) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["velocity"] = std::to_string(velocity);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus send_aftertouch(int track_id, int channel, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus send_pitch_bend(int track_id, int channel, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus send_modulation(int track_id, int channel, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return _return_status;
    }
};

class AudioGraphControllerMockup : public AudioGraphController, public TestableController
{
public:
    std::vector<ProcessorInfo> get_all_processors() const override {return processors;}

    std::vector<TrackInfo> get_all_tracks() const override {return tracks;}

    std::pair<ControlStatus, int> get_track_id(const std::string& /*track_name*/) const override
    {
        return {_return_status, track1.id};
    }

    std::pair<ControlStatus, TrackInfo> get_track_info(int /*track_id*/) const override
    {
        return {_return_status, track1};
    }

    std::pair<ControlStatus, std::vector<ProcessorInfo>> get_track_processors(int /*track_id*/) const override
    {
        return {_return_status, processors};
    }

    std::pair<ControlStatus, int> get_processor_id(const std::string& /*processor_name*/) const override
    {
        return {_return_status, processor_1.id};
    }

    std::pair<ControlStatus, ProcessorInfo> get_processor_info(int /*processor_id*/) const override
    {
        return {_return_status, processor_1};
    }

    std::pair<ControlStatus, bool> get_processor_bypass_state(int /*processor_id*/) const override
    {
        return {_return_status, DEFAULT_BYPASS_STATE};
    }

    std::pair<ControlStatus, ext::ProcessorState> get_processor_state(int /*processor_id*/) const override
    {
        return {_return_status, ext::ProcessorState()};
    }

    ControlStatus set_processor_bypass_state(int processor_id, bool bypass_enabled) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["bypass enabled"] = std::to_string(bypass_enabled);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus set_processor_state(int processor_id, const ext::ProcessorState& /*state*/) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus create_track(const std::string& name, int channels) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["channels"] = std::to_string(channels);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus create_multibus_track(const std::string& name, int buses) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["buses"] = std::to_string(buses);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus create_pre_track(const std::string& name) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _recently_called = true;
        return _return_status;
    }

    ControlStatus create_post_track(const std::string& name) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _recently_called = true;
        return _return_status;
    }


    ControlStatus move_processor_on_track(int processor_id,
                                          int source_track_id,
                                          int dest_track_id,
                                          std::optional<int> before_processor_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor_id"] = std::to_string(processor_id);
        _args_from_last_call["source_track_id"] = std::to_string(source_track_id);
        _args_from_last_call["dest_track_id"] = std::to_string(dest_track_id);
        _args_from_last_call["before_processor_id"] = std::to_string(before_processor_id.value_or(-1));
        _recently_called = true;
        return _return_status;
    }

    ControlStatus create_processor_on_track(const std::string& name,
                                            const std::string& uid,
                                            const std::string& file,
                                            PluginType type,
                                            int track_id,
                                            std::optional<int> before_processor_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["uid"] = uid;
        _args_from_last_call["file"] = file;
        _args_from_last_call["type"] = std::to_string(static_cast<int>(type));
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _args_from_last_call["before_processor_id"] = std::to_string(before_processor_id.value_or(-1));
        _recently_called = true;
        return _return_status;
    }

    ControlStatus delete_processor_from_track(int processor_id, int track_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor_id"] = std::to_string(processor_id);
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _recently_called = true;
        return _return_status;
    }

    ControlStatus delete_track(int track_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _recently_called = true;
        return _return_status;
    }
};

class ProgramControllerMockup : public ProgramController, public TestableController
{
public:
    std::pair<ControlStatus, int> get_processor_current_program(int /*processor_id*/) const override
    {
        return {_return_status, DEFAULT_PROGRAM_ID};
    }

    std::pair<ControlStatus, std::string> get_processor_current_program_name(int /*processor_id*/) const override
    {
        return {_return_status, DEFAULT_PROGRAM_NAME};
    }

    std::pair<ControlStatus, std::string> get_processor_program_name(int /*processor_id*/, int /*program_id*/) const override
    {
        return {_return_status, DEFAULT_PROGRAM_NAME};
    }

    std::pair<ControlStatus, std::vector<std::string>> get_processor_programs(int /*processor_id*/) const override
    {
        return {_return_status, DEFAULT_PROGRAMS};
    }

    ControlStatus set_processor_program(int processor_id, int program_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["program id"] = std::to_string(program_id);
        _recently_called = true;
        return _return_status;
    }
};

class ParameterControllerMockup : public ParameterController, public TestableController
{
    std::pair<ControlStatus, std::vector<ParameterInfo>> get_processor_parameters(int /*processor_id*/) const override
    {
        return {_return_status, parameters};
    }

    std::pair<ControlStatus, std::vector<ParameterInfo>> get_track_parameters(int /*processor_id*/) const override
    {
        return {_return_status, parameters};
    }

    std::pair<ControlStatus, int> get_parameter_id(int /*processor_id*/, const std::string& /*parameter*/) const override
    {
        return {_return_status, parameter_1.id};
    }

    std::pair<ControlStatus, ParameterInfo> get_parameter_info(int /*processor_id*/, int /*parameter_id*/) const override
    {
        return {_return_status, parameter_1};
    }

    std::pair<ControlStatus, float> get_parameter_value(int /*processor_id*/, int /*parameter_id*/) const override
    {
        return {_return_status, DEFAULT_PARAMETER_VALUE};
    }

    std::pair<ControlStatus, float> get_parameter_value_in_domain(int /*processor_id*/, int /*parameter_id*/) const override
    {
        return {_return_status, DEFAULT_PARAMETER_VALUE};
    }

    std::pair<ControlStatus, std::string> get_parameter_value_as_string(int /*processor_id*/, int /*parameter_id*/) const override
    {
        return {_return_status, std::to_string(DEFAULT_PARAMETER_VALUE)};
    }

    ControlStatus set_parameter_value(int processor_id, int parameter_id, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["parameter id"] = std::to_string(parameter_id);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return _return_status;
    }

    std::pair<ControlStatus, std::vector<PropertyInfo>> get_processor_properties(int /*processor_id*/) const override
    {
        return {ControlStatus::OK, std::vector<PropertyInfo>({property_1})};
    }

    std::pair<ControlStatus, std::vector<PropertyInfo>> get_track_properties(int /*processor_id*/) const override
    {
        return {ControlStatus::OK, std::vector<PropertyInfo>({property_1})};
    }

    std::pair<ControlStatus, int> get_property_id(int /*processor_id*/, const std::string& /*property_id*/) const override
    {
        return {ControlStatus::OK, 0};
    }
    std::pair<ControlStatus, PropertyInfo> get_property_info(int /*processor_id*/, int /*property_id*/) const override
    {
        return {ControlStatus::OK, property_1};
    }

    std::pair<ControlStatus, std::string> get_property_value(int /*processor_id*/, int /*property_id*/) const override
    {
        return {_return_status, DEFAULT_STRING_PROPERTY};
    }

    ControlStatus set_property_value(int processor_id, int property_id, const std::string& value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["property id"] = std::to_string(property_id);
        _args_from_last_call["value"] = value;
        _recently_called = true;
        return _return_status;
    }
};

class MidiControllerMockup : public MidiController, public TestableController
{
public:
    int get_input_ports() const override {return 0;}

    int get_output_ports() const override {return 0;}

    std::vector<MidiKbdConnection> get_all_kbd_input_connections() const override
    {
        return std::vector<MidiKbdConnection>();
    }

    std::vector<MidiKbdConnection> get_all_kbd_output_connections() const override
    {
        return std::vector<MidiKbdConnection>();
    }

    std::vector<MidiCCConnection> get_all_cc_input_connections() const override
    {
        return std::vector<MidiCCConnection>();
    }

    std::vector<MidiPCConnection> get_all_pc_input_connections() const override
    {
        return std::vector<MidiPCConnection>();
    }

    std::pair<ControlStatus, std::vector<MidiCCConnection>>
    get_cc_input_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<MidiCCConnection>()};
    }

    std::pair<ControlStatus, std::vector<MidiPCConnection>>
    get_pc_input_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<MidiPCConnection>()};
    }

    bool get_midi_clock_output_enabled(int /*port*/) const override
    {
        return false;
    }

    ControlStatus set_midi_clock_output_enabled(bool /*enabled*/, int /*port*/) override
    {
        return _return_status;
    }

    ControlStatus connect_kbd_input_to_track(int /*track_id*/, MidiChannel /*channel*/, int /*port*/, bool /*raw_midi*/) override
    {
        return _return_status;
    }

    ControlStatus connect_kbd_output_from_track(int /*track_id*/, MidiChannel /*channel*/, int /*port*/) override
    {
        return _return_status;
    }

    ControlStatus connect_cc_to_parameter(int /*processor_id*/,
                                          int /*parameter_id*/,
                                          MidiChannel /*channel*/,
                                          int /*port*/,
                                          int /*cc_number*/,
                                          float /*min_range*/,
                                          float /*max_range*/,
                                          bool /*relative_mode*/) override
    {
        return _return_status;
    }

    ControlStatus connect_pc_to_processor(int /*processor_id*/, MidiChannel /*channel*/, int /*port*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_kbd_input(int /*track_id*/, MidiChannel /*channel*/, int /*port*/, bool /*raw_midi*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_kbd_output(int /*track_id*/, MidiChannel /*channel*/, int /*port*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_cc(int /*processor_id*/, MidiChannel /*channel*/, int /*port*/, int /*cc_number*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_pc(int /*processor_id*/, MidiChannel /*channel*/, int /*port*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_cc_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_pc_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }
};

class AudioRoutingControllerMockup : public AudioRoutingController, public TestableController
{
public:
    std::vector<AudioConnection> get_all_input_connections() const override
    {
        return std::vector<AudioConnection>();
    }

    std::vector<AudioConnection> get_all_output_connections() const override
    {
        return std::vector<AudioConnection>();
    }

    std::vector<AudioConnection> get_input_connections_for_track(int /*track_id*/) const override
    {
        return std::vector<AudioConnection>();
    }

    std::vector<AudioConnection> get_output_connections_for_track(int /*track_id*/) const override
    {
        return std::vector<AudioConnection>();
    }

    ControlStatus connect_input_channel_to_track(int /*track_id*/, int /*track_channel*/, int /*input_channel*/) override
    {
        return _return_status;
    }

    ControlStatus connect_output_channel_to_track(int /*track_id*/, int /*track_channel*/, int /*output_channel*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_input(int /*track_id*/, int /*track_channel*/, int /*input_channel*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_output(int /*track_id*/, int /*track_channel*/, int /*output_channel*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_inputs_from_track(int /*track_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_outputs_from_track(int /*track_id*/) override
    {
        return _return_status;
    }
};

class CvGateControllerMockup : public CvGateController, public TestableController
{
public:
    int get_cv_input_ports() const override {return 0;}

    int get_cv_output_ports() const override {return 0;}

    std::vector<CvConnection> get_all_cv_input_connections() const override
    {
        return std::vector<CvConnection>();
    }

    std::vector<CvConnection> get_all_cv_output_connections() const override
    {
        return std::vector<CvConnection>();
    }

    std::vector<GateConnection> get_all_gate_input_connections() const override
    {
        return std::vector<GateConnection>();
    }

    std::vector<GateConnection> get_all_gate_output_connections() const override
    {
        return std::vector<GateConnection>();
    }

    std::pair<ControlStatus, std::vector<CvConnection>>
    get_cv_input_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<CvConnection>()};
    }

    std::pair<ControlStatus, std::vector<CvConnection>>
    get_cv_output_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<CvConnection>()};
    }

    std::pair<ControlStatus, std::vector<GateConnection>>
    get_gate_input_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<GateConnection>()};
    }

    std::pair<ControlStatus, std::vector<GateConnection>>
    get_gate_output_connections_for_processor(int /*processor_id*/) const override
    {
        return {_return_status, std::vector<GateConnection>()};
    }

    ControlStatus connect_cv_input_to_parameter(int /*processor_id*/, int /*parameter_id*/, int /*cv_input_id*/) override
    {
        return _return_status;
    }

    ControlStatus connect_cv_output_from_parameter(int /*processor_id*/, int /*parameter_id*/, int /*cv_output_id*/) override
    {
        return _return_status;
    }

    ControlStatus connect_gate_input_to_processor(int /*processor_id*/, int /*gate_input_id*/, int /*channel*/, int /*note_no*/) override
    {
        return _return_status;
    }

    ControlStatus connect_gate_output_from_processor(int /*processor_id*/, int /*gate_output_id*/, int /*channel*/, int /*note_no*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_cv_input(int /*processor_id*/, int /*parameter_id*/, int /*cv_input_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_cv_output(int /*processor_id*/, int /*parameter_id*/, int /*cv_output_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_gate_input(int /*processor_id*/, int /*gate_input_id*/, int /*channel*/, int /*note_no*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_gate_output(int /*processor_id*/, int /*gate_output_id*/, int /*channel*/, int /*note_no*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_cv_inputs_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_cv_outputs_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_gate_inputs_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }

    ControlStatus disconnect_all_gate_outputs_from_processor(int /*processor_id*/) override
    {
        return _return_status;
    }
};

class OscControllerMockup : public OscController, public TestableController
{
public:
    std::string get_send_ip() const override {return "";}

    int get_send_port() const override {return 0;}

    int get_receive_port() const override {return 0;}

    std::vector<std::string> get_enabled_parameter_outputs() const override
    {
        return std::vector<std::string>();
    }

    ControlStatus enable_output_for_parameter(int /*processor_id*/, int /*parameter_id*/) override
    {
        return _return_status;
    }

    ControlStatus disable_output_for_parameter(int /*processor_id*/, int /*parameter_id*/) override
    {
        return _return_status;
    }

    ControlStatus enable_all_output() override
    {
        return _return_status;
    }

    ControlStatus disable_all_output() override
    {
        return _return_status;
    }
};

class SessionControllerMockup : public SessionController, public TestableController
{
public:
    ext::SessionState save_session() const override
    {
        return SessionState();
    }

    ext::ControlStatus restore_session(const ext::SessionState& /*state*/) override
    {
        return ControlStatus::UNSUPPORTED_OPERATION;
    }
};


class ControlMockup : public SushiControl
{
public:
    ControlMockup() : SushiControl(&_system_controller_mockup,
                                   &_transport_controller_mockup,
                                   &_timing_controller_mockup,
                                   &_keyboard_controller_mockup,
                                   &_audio_graph_controller_mockup,
                                   &_program_controller_mockup,
                                   &_parameter_controller_mockup,
                                   &_midi_controller_mockup,
                                   &_audio_routing_controller_mockup,
                                   &_cv_gate_controller_mockup,
                                   &_osc_controller_mockup,
                                   &_session_controller_mockup) {}

    ControlStatus subscribe_to_notifications(NotificationType /* type */, ControlListener* /* listener */) override
    {
        return ControlStatus::OK;
    }

    bool was_recently_called()
    {
        for (auto c : _controllers)
        {
            if (c->was_recently_called())
            {
                return true;
            }
        }
        return false;
    }

    void clear_recent_call()
    {
        for (auto c : _controllers)
        {
            c->clear_recent_call();
        }
    }

    SystemControllerMockup* system_controller_mockup()
    {
        return &_system_controller_mockup;
    }

    TransportControllerMockup* transport_controller_mockup()
    {
        return &_transport_controller_mockup;
    }

    TimingControllerMockup* timing_controller_mockup()
    {
        return &_timing_controller_mockup;
    }

    KeyboardControllerMockup* keyboard_controller_mockup()
    {
        return &_keyboard_controller_mockup;
    }

    AudioGraphControllerMockup* audio_graph_controller_mockup()
    {
        return &_audio_graph_controller_mockup;
    }

    ProgramControllerMockup* program_controller_mockup()
    {
        return &_program_controller_mockup;
    }

    ParameterControllerMockup* parameter_controller_mockup()
    {
        return &_parameter_controller_mockup;
    }

    MidiControllerMockup* midi_controller_mockup()
    {
        return &_midi_controller_mockup;
    }

    AudioRoutingControllerMockup* audio_routing_controller_mockup()
    {
        return &_audio_routing_controller_mockup;
    }

    CvGateControllerMockup* cv_gate_controller_mockup()
    {
        return &_cv_gate_controller_mockup;
    }

    OscControllerMockup* osc_controller_mockup()
    {
        return &_osc_controller_mockup;
    }

    SessionControllerMockup* session_controller_mockup()
    {
        return &_session_controller_mockup;
    }

private:

    SystemControllerMockup       _system_controller_mockup;
    TransportControllerMockup    _transport_controller_mockup;
    TimingControllerMockup       _timing_controller_mockup;
    KeyboardControllerMockup     _keyboard_controller_mockup;
    AudioGraphControllerMockup   _audio_graph_controller_mockup;
    ProgramControllerMockup      _program_controller_mockup;
    ParameterControllerMockup    _parameter_controller_mockup;
    MidiControllerMockup         _midi_controller_mockup;
    AudioRoutingControllerMockup _audio_routing_controller_mockup;
    CvGateControllerMockup       _cv_gate_controller_mockup;
    OscControllerMockup          _osc_controller_mockup;
    SessionControllerMockup      _session_controller_mockup;

    std::vector<TestableController*> _controllers{&_system_controller_mockup,
                                                  &_transport_controller_mockup,
                                                  &_timing_controller_mockup,
                                                  &_keyboard_controller_mockup,
                                                  &_audio_graph_controller_mockup,
                                                  &_program_controller_mockup,
                                                  &_parameter_controller_mockup,
                                                  &_midi_controller_mockup,
                                                  &_audio_routing_controller_mockup,
                                                  &_cv_gate_controller_mockup,
                                                  &_osc_controller_mockup,
                                                  &_session_controller_mockup};
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_MOCKUP_H
