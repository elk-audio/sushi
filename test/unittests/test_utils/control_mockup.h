#ifndef SUSHI_CONTROL_MOCKUP_H
#define SUSHI_CONTROL_MOCKUP_H

#include <bits/stdc++.h> 

#include "control_interface.h"

namespace sushi {
namespace ext {

const ParameterInfo parameter1{0,ParameterType::INT,"param 1", "param 1", "unit", 0, 0, 0};
const ParameterInfo parameter2{1,ParameterType::FLOAT,"param 2", "param 2", "unit", 1, 1, 1};
const ParameterInfo parameter3{2,ParameterType::BOOL,"param 3", "param 3", "unit", -1, -1, -1};
const std::vector<ParameterInfo> parameters{parameter1, parameter2, parameter3};

const ProcessorInfo processor1{0, "proc 1", "proc 1", 0 ,0};
const ProcessorInfo processor2{1, "proc 2", "proc 2", 1 ,1};
const std::vector<ProcessorInfo> processors{processor1, processor2};

const TrackInfo track1{0,"track1","track1",0,0,0,0,{}};
const TrackInfo track2{1,"track2","track2",1,1,1,1,{}};
const std::vector<TrackInfo> tracks{track1, track2};

constexpr float default_samplerate = 48000.0f;
constexpr float default_tempo = 120.0f;
constexpr float default_parameter_value = 0.745f;
constexpr auto default_string_property = "string property";
constexpr bool default_timing_statistics_enabled = false;
constexpr bool default_bypass_state = false;
constexpr PlayingMode default_playing_mode = PlayingMode::PLAYING;
constexpr SyncMode default_sync_mode = SyncMode::INTERNAL;
constexpr TimeSignature default_time_signature = TimeSignature{4,4};
constexpr ControlStatus default_control_status = ControlStatus::OK;
constexpr CpuTimings default_timings = CpuTimings{1.0f, 0.5f, 1.5f};
constexpr int default_program_id = 1;
constexpr auto default_program_name = "program 1";
const std::vector<std::string> default_programs = {default_program_name, "program 2"};

class ControlMockup : public SushiControl
{
public:

     // Main engine controls
    virtual float get_samplerate() const override { return default_samplerate; };

    virtual PlayingMode get_playing_mode() const override { return default_playing_mode; };

    virtual void set_playing_mode(PlayingMode playing_mode) override
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

    virtual SyncMode get_sync_mode() const override
    {
        return default_sync_mode;
    };

    virtual void set_sync_mode(SyncMode sync_mode) override
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

    virtual float get_tempo() const override { return default_tempo; };

    virtual ControlStatus set_tempo(float tempo) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["tempo"] = std::to_string(tempo);
        _recently_called = true;
        return default_control_status; 
    };

    virtual TimeSignature get_time_signature() const override { return default_time_signature; };

    virtual ControlStatus set_time_signature(TimeSignature signature) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["numerator"] = std::to_string(signature.numerator);
        _args_from_last_call["denominator"] = std::to_string(signature.denominator);
        _recently_called = true;
        return default_control_status; 
    };

    virtual bool get_timing_statistics_enabled() const override
    {
        return default_timing_statistics_enabled;
    };

    virtual void set_timing_statistics_enabled(bool enabled) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["enabled"] = std::to_string(enabled);
        _recently_called = true;
    };

    virtual std::vector<TrackInfo> get_tracks() const override { return tracks; };

    // Keyboard control
    virtual ControlStatus send_note_on(int track_id, int channel, int note, float velocity) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["velocity"] = std::to_string(velocity);
        _recently_called = true;
        return default_control_status; 
    };

    virtual ControlStatus send_note_off(int track_id, int channel, int note, float velocity) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["velocity"] = std::to_string(velocity);
        _recently_called = true;
        return default_control_status; 
    };

    virtual ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["note"] = std::to_string(note);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return default_control_status; 
    };

    virtual ControlStatus send_aftertouch(int track_id, int channel, float value) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return default_control_status;
    };

    virtual ControlStatus send_pitch_bend(int track_id, int channel, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return default_control_status;
    };

    virtual ControlStatus send_modulation(int track_id, int channel, float value) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _args_from_last_call["channel"] = std::to_string(channel);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return default_control_status;
    };

    // Cpu Timings
    virtual std::pair<ControlStatus, CpuTimings> get_engine_timings() const override
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };

    virtual std::pair<ControlStatus, CpuTimings> get_track_timings(int /* track_id */) const override
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };

    virtual std::pair<ControlStatus, CpuTimings> get_processor_timings(int /* processor_id */) const override
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };

    virtual ControlStatus reset_all_timings() override
    { 
        _recently_called = true;
        return default_control_status; 
    };

    virtual ControlStatus reset_track_timings(int track_id) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["track id"] = std::to_string(track_id);
        _recently_called = true;
        return default_control_status;
    };

    virtual ControlStatus reset_processor_timings(int processor_id) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _recently_called = true;
        return default_control_status; 
    };

    // Track control
    virtual std::pair<ControlStatus, int> get_track_id(const std::string& /* track_name */) const override
    { 
        return std::pair<ControlStatus, int>(default_control_status, track1.id);
    };

    virtual std::pair<ControlStatus, TrackInfo> get_track_info(int /* track_id */) const override
    {
        return std::pair<ControlStatus, TrackInfo>(default_control_status, track1);
    };

    virtual std::pair<ControlStatus, std::vector<ProcessorInfo>> get_track_processors(int /* track_id */) const override 
    {
        return std::pair<ControlStatus, std::vector<ProcessorInfo>>(ControlStatus::OK, processors);
    };

    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_track_parameters(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, std::vector<ParameterInfo>>(default_control_status, parameters);
    };

    // Processor control
    virtual std::pair<ControlStatus, int> get_processor_id(const std::string& /* processor_name */) const override
    {
        return std::pair<ControlStatus, int>(default_control_status, processor1.id);
    };

    virtual std::pair<ControlStatus, ProcessorInfo> get_processor_info(int /* processor_id */) const override
    {
        return std::pair<ControlStatus, ProcessorInfo>(default_control_status, processor1);
    };

    virtual std::pair<ControlStatus, bool> get_processor_bypass_state(int /* processor_id */) const override
    {
        return std::pair<ControlStatus, bool>(default_control_status, default_bypass_state);
    };

    virtual ControlStatus set_processor_bypass_state(int processor_id, bool bypass_enabled) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["bypass enabled"] = std::to_string(bypass_enabled);
        _recently_called = true;
        return default_control_status; 
    };

    virtual std::pair<ControlStatus, int> get_processor_current_program(int /* processor_id */) const override
    {
        return std::pair<ControlStatus, int>(default_control_status, default_program_id);
    };

    virtual std::pair<ControlStatus, std::string> get_processor_current_program_name(int /* processor_id */) const override
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_program_name);
    };

    virtual std::pair<ControlStatus, std::string> get_processor_program_name(int /* processor_id */, int /* program_id */) const override
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_program_name);
    };

    virtual std::pair<ControlStatus, std::vector<std::string>> get_processor_programs(int /* processor_id */) const override
    {
        return std::pair<ControlStatus, std::vector<std::string>>(default_control_status, default_programs);
    };

    virtual ControlStatus set_processor_program(int processor_id, int program_id) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["program id"] = std::to_string(program_id);
        _recently_called = true;
        return default_control_status; 
    };

    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_processor_parameters(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, std::vector<ParameterInfo>>(ControlStatus::OK, parameters);
    };

    // Parameter control
    virtual std::pair<ControlStatus, int> get_parameter_id(int /* processor_id */, const std::string& /* parameter */) const override
    {
        return std::pair<ControlStatus, int>(default_control_status, parameter1.id);
    };

    virtual std::pair<ControlStatus, ParameterInfo> get_parameter_info(int /* processor_id */, int /* parameter_id */) const override
    {
        return std::pair<ControlStatus, ParameterInfo>(default_control_status, parameter1);
    };

    virtual std::pair<ControlStatus, float> get_parameter_value(int /* processor_id */, int /* parameter_id */) const override
    {
        return std::pair<ControlStatus, float>(default_control_status, default_parameter_value);
    };

    virtual std::pair<ControlStatus, float> get_parameter_value_in_domain(int /* processor_id */, int /* parameter_id */) const override
    {
        return std::pair<ControlStatus, float>(default_control_status, default_parameter_value);
    };

    virtual std::pair<ControlStatus, std::string> get_parameter_value_as_string(int /* processor_id */, int /* parameter_id */) const override
    {
        return std::pair<ControlStatus, std::string>(default_control_status, std::to_string(default_parameter_value));
    };

    virtual std::pair<ControlStatus, std::string> get_string_property_value(int /* processor_id */, int /* parameter_id */) const override
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_string_property);
    };

    virtual ControlStatus set_parameter_value(int processor_id, int parameter_id, float value) override
    { 
        _args_from_last_call.clear();
        _args_from_last_call["processor id"] = std::to_string(processor_id);
        _args_from_last_call["parameter id"] = std::to_string(parameter_id);
        _args_from_last_call["value"] = std::to_string(value);
        _recently_called = true;
        return default_control_status; 
    };

    virtual ControlStatus set_string_property_value(int /* processor_id */, int /* parameter_id */, const std::string& /* value */) override
    {
        return default_control_status;
    };

    ControlStatus create_stereo_track(const std::string& name, int output_bus, std::optional<int> input_bus) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["output_bus"] = std::to_string(output_bus);
        _args_from_last_call["input_bus"] = std::to_string(input_bus.value_or(-1));
        _recently_called = true;
        return default_control_status;
    }

    ControlStatus create_mono_track(const std::string& name, int output_channel, std::optional<int> input_channel) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["output_channel"] = std::to_string(output_channel);
        _args_from_last_call["input_channel"] = std::to_string(input_channel.value_or(-1));
        _recently_called = true;
        return default_control_status;
    }

    ControlStatus delete_track(int track_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _recently_called = true;
        return default_control_status;
    }


    ControlStatus create_processor_on_track(const std::string& name, const std::string& uid, const std::string& file,
                                            PluginType type, int track_id, std::optional<int> before_processor_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["name"] = name;
        _args_from_last_call["uid"] = uid;
        _args_from_last_call["file"] = file;
        _args_from_last_call["type"] = std::to_string(static_cast<int>(type));
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _args_from_last_call["before_processor_id"] = std::to_string(before_processor_id.value_or(-1));
        _recently_called = true;
        return default_control_status;
    }

    ControlStatus move_processor_on_track(int processor_id, int source_track_id, int dest_track_id, std::optional<int> before_processor) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor_id"] = std::to_string(processor_id);
        _args_from_last_call["source_track_id"] = std::to_string(source_track_id);
        _args_from_last_call["dest_track_id"] = std::to_string(dest_track_id);
        _args_from_last_call["before_processor_id"] = std::to_string(before_processor.value_or(-1));
        _recently_called = true;
        return default_control_status;
    }

    ControlStatus delete_processor_from_track(int processor_id, int track_id) override
    {
        _args_from_last_call.clear();
        _args_from_last_call["processor_id"] = std::to_string(processor_id);
        _args_from_last_call["track_id"] = std::to_string(track_id);
        _recently_called = true;
        return default_control_status;
    }

    std::unordered_map<std::string,std::string> get_args_from_last_call()
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

private:
    std::unordered_map<std::string,std::string> _args_from_last_call;
    bool _recently_called{false};
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_MOCKUP_H