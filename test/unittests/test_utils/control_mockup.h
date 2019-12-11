#ifndef SUSHI_CONTROL_MOCKUP_H
#define SUSHI_CONTROL_MOCKUP_H

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

const TrackInfo track1{0,"track1","track1",0,0,0,0,0};
const TrackInfo track2{1,"track2","track2",1,1,1,1,1};
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
    virtual float                               get_samplerate() const override { return default_samplerate; };
    virtual PlayingMode                         get_playing_mode() const override { return default_playing_mode; };
    virtual void                                set_playing_mode(PlayingMode /* playing_mode */) override {};
    virtual SyncMode                            get_sync_mode() const override { return default_sync_mode; };
    virtual void                                set_sync_mode(SyncMode /* sync_mode */) override {};
    virtual float                               get_tempo() const override { return default_tempo; };
    virtual ControlStatus                       set_tempo(float /* tempo */) override { return default_control_status; };
    virtual TimeSignature                       get_time_signature() const override { return default_time_signature; };
    virtual ControlStatus                       set_time_signature(TimeSignature /* signature */) override { return default_control_status; };
    virtual bool                                get_timing_statistics_enabled() override { return default_timing_statistics_enabled; };
    virtual void                                set_timing_statistics_enabled(bool /* enabled */) const override {};
    virtual std::vector<TrackInfo>              get_tracks() const override { return tracks; };

    // Keyboard control
    virtual ControlStatus                       send_note_on(int /* track_id */, int /* channel */, int /* note */, float /* velocity */) override { return default_control_status; };
    virtual ControlStatus                       send_note_off(int /* track_id */, int /* channel */, int /* note */, float /* velocity */) override { return default_control_status; };
    virtual ControlStatus                       send_note_aftertouch(int /* track_id */, int /* channel */, int /* note */, float /* value */) override { return default_control_status; };
    virtual ControlStatus                       send_aftertouch(int /* track_id */, int /* channel */, float /* value */) override { return default_control_status; };
    virtual ControlStatus                       send_pitch_bend(int /* track_id */, int /* channel */, float /* value */) override { return default_control_status; };
    virtual ControlStatus                       send_modulation(int /* track_id */, int /* channel */, float /* value */) override { return default_control_status; };

    // Cpu Timings
    virtual std::pair<ControlStatus, CpuTimings>    get_engine_timings() const override 
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };
    virtual std::pair<ControlStatus, CpuTimings>    get_track_timings(int /* track_id */) const override 
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };
    virtual std::pair<ControlStatus, CpuTimings>    get_processor_timings(int /* processor_id */) const override 
    { 
        return std::pair<ControlStatus, CpuTimings>(default_control_status, default_timings); 
    };
    virtual ControlStatus                           reset_all_timings() override { return default_control_status; };
    virtual ControlStatus                           reset_track_timings(int /* track_id */) override { return default_control_status; };
    virtual ControlStatus                           reset_processor_timings(int /* processor_id */) override { return default_control_status; };

    // Track control
    virtual std::pair<ControlStatus, int>           get_track_id(const std::string& /* track_name */) const override 
    { 
        return std::pair<ControlStatus, int>(default_control_status, track1.id);
    };
    virtual std::pair<ControlStatus, TrackInfo>     get_track_info(int /* track_id */) const override 
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
    virtual std::pair<ControlStatus, int>              get_processor_id(const std::string& /* processor_name */) const override 
    {
        return std::pair<ControlStatus, int>(default_control_status, processor1.id);
    };
    virtual std::pair<ControlStatus, ProcessorInfo>    get_processor_info(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, ProcessorInfo>(default_control_status, processor1);
    };
    virtual std::pair<ControlStatus, bool>             get_processor_bypass_state(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, bool>(default_control_status, default_bypass_state);
    };
    virtual ControlStatus                              set_processor_bypass_state(int /* processor_id */, bool /* bypass_enabled */) override { return default_control_status; };
    virtual std::pair<ControlStatus, int>              get_processor_current_program(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, int>(default_control_status, default_program_id);
    };
    virtual std::pair<ControlStatus, std::string>      get_processor_current_program_name(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_program_name);
    };
    virtual std::pair<ControlStatus, std::string>      get_processor_program_name(int /* processor_id */, int /* program_id */) const override 
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_program_name);
    };
    virtual std::pair<ControlStatus, std::vector<std::string>>   get_processor_programs(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, std::vector<std::string>>(default_control_status, default_programs);
    };
    virtual ControlStatus                              set_processor_program(int /* processor_id */, int /* program_id */)override { return default_control_status; };
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_processor_parameters(int /* processor_id */) const override 
    {
        return std::pair<ControlStatus, std::vector<ParameterInfo>>(ControlStatus::OK, parameters);
    };

    // Parameter control
    virtual std::pair<ControlStatus, int>              get_parameter_id(int /* processor_id */, const std::string& /* parameter */) const override 
    {
        return std::pair<ControlStatus, int>(default_control_status, parameter1.id);
    };
    virtual std::pair<ControlStatus, ParameterInfo>    get_parameter_info(int /* processor_id */, int /* parameter_id */) const override 
    {
        return std::pair<ControlStatus, ParameterInfo>(default_control_status, parameter1);
    };
    virtual std::pair<ControlStatus, float>            get_parameter_value(int /* processor_id */, int /* parameter_id */) const override 
    {
        return std::pair<ControlStatus, float>(default_control_status, default_parameter_value);
    };
    virtual std::pair<ControlStatus, float>            get_parameter_value_normalised(int /* processor_id */, int /* parameter_id */) const override 
    {
        return std::pair<ControlStatus, float>(default_control_status, default_parameter_value);
    };
    virtual std::pair<ControlStatus, std::string>      get_parameter_value_as_string(int /* processor_id */, int /* parameter_id */) const override 
    {
        return std::pair<ControlStatus, std::string>(default_control_status, std::to_string(default_parameter_value));
    };
    virtual std::pair<ControlStatus, std::string>      get_string_property_value(int /* processor_id */, int /* parameter_id */) const override 
    {
        return std::pair<ControlStatus, std::string>(default_control_status, default_string_property);
    };
    virtual ControlStatus                              set_parameter_value(int /* processor_id */, int /* parameter_id */, float /* value */) override { return default_control_status; };
    virtual ControlStatus                              set_parameter_value_normalised(int /* processor_id */, int /* parameter_id */, float /* value */) override { return default_control_status; };
    virtual ControlStatus                              set_string_property_value(int /* processor_id */, int /* parameter_id */, const std::string& /* value */) override { return default_control_status; };
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_MOCKUP_H
