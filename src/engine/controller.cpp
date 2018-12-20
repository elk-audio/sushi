#include "engine/controller.h"
#include "engine/base_engine.h"

#include "logging.h"


MIND_GET_LOGGER_WITH_MODULE_NAME("controller")

namespace sushi {

Controller::Controller(engine::BaseEngine* engine) : _engine{engine}
{
    _event_dispatcher = _engine->event_dispatcher();
}

Controller::~Controller()
{

}

float Controller::get_samplerate() const
{
    MIND_LOG_INFO("get_samplerate called, returning 48000 ");
    return 48000.0f;
}

ext::PlayingMode Controller::get_playing_mode() const
{
    MIND_LOG_INFO("get_playing_mode called, returning ");

    return ext::PlayingMode::PLAYING;
}

void Controller::set_playing_mode(ext::PlayingMode playing_mode)
{
    MIND_LOG_INFO("set_playing_mode called, returning ");
}

ext::SyncMode Controller::get_sync_mode() const
{
    MIND_LOG_INFO("get_sync_mode called, returning ");
    return ext::SyncMode::INTERNAL;
}

void Controller::set_sync_mode(ext::SyncMode sync_mode)
{
    MIND_LOG_INFO("set_sync_mode called, returning ");
}

float Controller::get_tempo() const
{
    MIND_LOG_INFO("get_samplerate called, returning 120 ");
    return 120;
}

ext::ControlStatus Controller::set_tempo(float tempo)
{
    MIND_LOG_INFO("set_tempo called, returning ");
    return ext::ControlStatus::OK;
}

ext::TimeSignature Controller::get_time_signature() const
{
    MIND_LOG_INFO("get_time_signature called, returning ");
    return {4,4};
}

ext::ControlStatus Controller::set_time_signature(ext::TimeSignature signature)
{
    MIND_LOG_INFO("set_time_signature called, returning ");
    return ext::ControlStatus::OK;
}

bool Controller::get_timing_statistics_enabled()
{
    MIND_LOG_INFO("get_timing_statistics_enabled called, returning ");
    return true;
}

void Controller::set_timing_statistics_enabled(bool enabled) const
{
    MIND_LOG_INFO("set_timing_statistics_enabled called, returning ");
}

std::vector<ext::TrackInfo> Controller::get_tracks() const
{
    auto& tracks = _engine->all_tracks();
    std::vector<ext::TrackInfo> returns;
    for (const auto& t : tracks)
    {
        ext::TrackInfo info;
        info.id = t->id();
        info.name = t->name();
        info.label = t->label();
        info.input_busses = t->input_busses();
        info.input_channels = t->input_channels();
        info.output_busses = t->output_channels();
        info.output_channels = t->output_channels();
        info.processor_count = t->process_chain().size();
        returns.push_back(info);
    }
    return returns;
}

ext::ControlStatus Controller::send_note_on(int track_id, int note, float velocity)
{
    MIND_LOG_INFO("send_note_on called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_note_off(int track_id, int note, float velocity)
{
    MIND_LOG_INFO("send_note_off called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_note_aftertouch(int track_id, int note, float value)
{
    MIND_LOG_INFO("send_note_aftertouch called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_aftertouch(int track_id, float value)
{
    MIND_LOG_INFO("send_aftertouch called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_pitch_bend(int track_id, float value)
{
    MIND_LOG_INFO("send_pitch_bend called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::send_modulation(int track_id, float value)
{
    MIND_LOG_INFO("send_modulation called, returning ");
    return ext::ControlStatus::OK;
}

ext::CpuTimings Controller::get_engine_timings() const
{
    MIND_LOG_INFO("get_engine_timings called, returning ");
    return {0.1, 0.2, 05};
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_track_timings(int track_id) const
{
    MIND_LOG_INFO("get_track_timings called, returning ");
    return {ext::ControlStatus::OK, ext::CpuTimings{0.05, 0.1, 0.02}};
}

std::pair<ext::ControlStatus, ext::CpuTimings> Controller::get_processor_timings(int processor_id) const
{
    MIND_LOG_INFO("get_processor_timings called, returning ");
    return {ext::ControlStatus::OK, ext::CpuTimings{0.05, 0.1, 0.02}};
}

ext::ControlStatus Controller::reset_all_timings()
{
    MIND_LOG_INFO("reset_all_timings called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::reset_track_timings(int track_id)
{
    MIND_LOG_INFO("reset_track_timings called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::reset_processor_timings(int processor_id)
{
    MIND_LOG_INFO("reset_processor_timings called, returning ");
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, int> Controller::get_track_id(const std::string& track_name) const
{
    MIND_LOG_INFO("get_track_id called, returning ");
    return {ext::ControlStatus::OK, 5};
}

std::pair<ext::ControlStatus, std::string> Controller::get_track_label(int track_id) const
{
    MIND_LOG_INFO("get_track_label called, returning ");
    return {ext::ControlStatus::OK, "track"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_track_name(int track_id) const
{
    MIND_LOG_INFO("get_track_name called, returning ");
    return {ext::ControlStatus::OK, "Track"};
}

std::pair<ext::ControlStatus, ext::TrackInfo> Controller::get_track_info(int track_id) const
{
    MIND_LOG_INFO("get_track_info called, returning ");
    ext::TrackInfo info;
    info.id = 5;
    info.label = "track";
    info.name = "Track";
    info.input_busses = 1;
    info.output_busses = 1;
    info.input_channels = 2;
    info.output_channels = 2;
    return {ext::ControlStatus::OK, info};
}

std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> Controller::get_track_processors(int track_id) const
{
    MIND_LOG_INFO("get_track_processors called for track: {}", track_id);

    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (track->id() == track_id)
        {
            std::vector<ext::ProcessorInfo> infos;
            const auto& procs = track->process_chain();
            for (const auto& processor : procs)
            {
                ext::ProcessorInfo info;
                info.label = processor->label();
                info.name = processor->name();
                info.id = processor->id();
                info.parameter_count = processor->parameter_count();
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ProcessorInfo>()};
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> Controller::get_track_parameters(int track_id) const
{
    const auto& tracks = _engine->all_tracks();
    for (const auto& track : tracks)
    {
        if (track->id() == track_id)
        {
            std::vector<ext::ParameterInfo> infos;
            const auto& params = track->all_parameters();
            for (const auto& param : params)
            {
                ext::ParameterInfo info;
                info.label = param->label();
                info.name = param->name();
                info.id = param->id();
                info.type = ext::ParameterType::FLOAT;
                info.min_range = 0;
                info.max_range = 1;
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> Controller::get_processor_id(const std::string& processor_name) const
{
    MIND_LOG_INFO("get_processor_id called, returning ");
    return {ext::ControlStatus::OK, 10};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_label(int processor_id) const
{
    MIND_LOG_INFO("get_processor_label called, returning ");
    return {ext::ControlStatus::OK, "processor"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_name(int processor_id) const
{
    MIND_LOG_INFO("get_processor_name called, returning ");
    return {ext::ControlStatus::OK, "Processor"};
}

std::pair<ext::ControlStatus, ext::ProcessorInfo> Controller::get_processor_info(int processor_id) const
{
    MIND_LOG_INFO("get_processor_info called, returning ");
    ext::ProcessorInfo info;
    info.id = 10;
    info.label = "processor";
    info.name = "Processor";
    info.parameter_count = 4;
    return {ext::ControlStatus::OK, info};
}

std::pair<ext::ControlStatus, bool> Controller::get_processor_bypass_state(int processor_id) const
{
    MIND_LOG_INFO("get_samplerate get_processor_bypass_state, returning ");
    return {ext::ControlStatus::OK, false};
}

ext::ControlStatus Controller::set_processor_bypass_state(int processor_id, bool bypass_enabled)
{
    MIND_LOG_INFO("set_processor_bypass_state called, returning ");
    return {ext::ControlStatus::OK};
}

std::pair<ext::ControlStatus, int> Controller::get_processor_current_program(int processor_id) const
{
    MIND_LOG_INFO("get_processor_current_program called, returning 15");
    return {ext::ControlStatus::OK, 15};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_current_program_name(int processor_id) const
{
    MIND_LOG_INFO("get_processor_current_program called, returning 15");
    return {ext::ControlStatus::OK, "program_x"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_processor_program_name(int processor_id, int program_id) const
{
    return {ext::ControlStatus::OK, "program_x"};
}

std::pair<ext::ControlStatus, std::vector<ext::ProgramInfo>> Controller::get_processor_programs(int processor_id) const
{
    return {ext::ControlStatus::OK, std::vector<ext::ProgramInfo>()};
}

ext::ControlStatus Controller::set_processor_program(int processor_id, int program_id)
{
    MIND_LOG_INFO("set_processor_program called, returning ");
    return ext::ControlStatus::OK;
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>>
Controller::get_processor_parameters(int processor_id) const
{
    const auto& procs = _engine->all_processors();
    for (const auto& proc : procs)
    {
        if (proc.second->id() == processor_id)
        {
            std::vector<ext::ParameterInfo> infos;
            const auto& params = proc.second->all_parameters();
            for (const auto& param : params)
            {
                ext::ParameterInfo info;
                info.label = param->label();
                info.name = param->name();
                info.id = param->id();
                info.type = ext::ParameterType::FLOAT;
                info.min_range = 0;
                info.max_range = 1;
                infos.push_back(info);
            }
            return {ext::ControlStatus::OK, infos};
        }
    }
    return {ext::ControlStatus::NOT_FOUND, std::vector<ext::ParameterInfo>()};
}

std::pair<ext::ControlStatus, int> Controller::get_parameter_id(int processor_id, const std::string& parameter) const
{
    MIND_LOG_INFO("get_parameter_id called, returning ");
    return {ext::ControlStatus::OK, 2};
}

std::pair<ext::ControlStatus, std::string> Controller::get_parameter_label(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_label called, returning ");
    return {ext::ControlStatus::OK, "parameter"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_parameter_name(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_name called, returning ");
    return {ext::ControlStatus::OK, "Parameter"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_parameter_unit(int processor_id, int parameter_id) const
{
    return {ext::ControlStatus::OK, "units"};
}

std::pair<ext::ControlStatus, ext::ParameterType> Controller::get_parameter_type(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_type called, returning ");
    return {ext::ControlStatus::OK, ext::ParameterType::FLOAT};
}

std::pair<ext::ControlStatus, ext::ParameterInfo> Controller::get_parameter_info(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_info called, returning ");
    ext::ParameterInfo info;
    info.id = 2;
    info.label = "parameter";
    info.name = "Parameter";
    info.type = ext::ParameterType::FLOAT;
    info.min_range = 0;
    info.max_range = 1;
    return {ext::ControlStatus::OK, info};
}

std::pair<ext::ControlStatus, float> Controller::get_parameter_value(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_value called, returning 5.5");
    return {ext::ControlStatus::OK, 5.5f};
}

std::pair<ext::ControlStatus, float> Controller::get_parameter_value_normalised(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_value_normalised called, returning 0.5");
    return {ext::ControlStatus::OK, 0.5f};}

std::pair<ext::ControlStatus, std::string> Controller::get_parameter_value_as_string(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_parameter_value_as_string called, returning ");
    return {ext::ControlStatus::OK, "0.5 units"};
}

std::pair<ext::ControlStatus, std::string> Controller::get_string_property_value(int processor_id, int parameter_id) const
{
    MIND_LOG_INFO("get_string_property_value called, returning ");
    return {ext::ControlStatus::OK, "helloworld"};
}

ext::ControlStatus Controller::set_parameter_value(int processor_id, int parameter_id, float value)
{
    auto event = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                          processor_id, parameter_id, value, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    MIND_LOG_INFO("set_parameter_value called, returning ");
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_parameter_value_normalised(int processor_id, int parameter_id, float value)
{
    return ext::ControlStatus::OK;
}

ext::ControlStatus Controller::set_string_property_value(int processor_id, int parameter_id, std::string value)
{
    MIND_LOG_INFO("set_string_property_value called, returning ");
    return ext::ControlStatus::OK;
}

}// namespace sushi