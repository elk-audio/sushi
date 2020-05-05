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
 * @brief Implementation of external control interface of sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "sub_controllers.h"

namespace sushi {
namespace engine {
namespace controller_impl {


std::string SystemController::get_interface_version() const
{
    return "";
}

std::string SystemController::get_sushi_version() const
{
    return "";
}

ext::SushiBuildInfo SystemController::get_sushi_buildinfo() const
{
    return ext::SushiBuildInfo();
}

int SystemController::get_input_audio_channel_count() const
{
    return 0;
}

int SystemController::get_output_audio_channel_count() const
{
    return 0;
}

float TransportController::get_samplerate() const
{
    return 0;
}

ext::PlayingMode TransportController::get_playing_mode() const
{
    return ext::PlayingMode::PLAYING;
}

ext::SyncMode TransportController::get_sync_mode() const
{
    return ext::SyncMode::INTERNAL;
}

ext::TimeSignature TransportController::get_time_signature() const
{
    return ext::TimeSignature();
}

float TransportController::get_tempo() const
{
    return 0;
}

void TransportController::set_sync_mode(ext::SyncMode sync_mode)
{

}

void TransportController::set_playing_mode(ext::PlayingMode playing_mode)
{

}

ext::ControlStatus TransportController::set_tempo(float tempo)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus TransportController::set_time_signature(ext::TimeSignature signature)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


bool TimingController::get_timing_statistics_enabled() const
{
    return false;
}

void TimingController::set_timing_statistics_enabled(bool enabled)
{

}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_engine_timings() const
{
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_track_timings(int track_id) const
{
}

std::pair<ext::ControlStatus, ext::CpuTimings> TimingController::get_processor_timings(int processor_id) const
{
}

ext::ControlStatus TimingController::reset_all_timings()
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus TimingController::reset_track_timings(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus TimingController::reset_processor_timings(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_note_on(int track_id, int channel, int note, float velocity)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_note_off(int track_id, int channel, int note, float velocity)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_note_aftertouch(int track_id, int channel, int note, float value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_aftertouch(int track_id, int channel, float value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_pitch_bend(int track_id, int channel, float value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus KeyboardController::send_modulation(int track_id, int channel, float value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


std::vector<ext::ProcessorInfo> AudioGraphController::get_all_processors() const
{
}

std::vector<ext::TrackInfo> AudioGraphController::get_all_tracks() const
{
}

std::pair<ext::ControlStatus, int> AudioGraphController::get_track_id(const std::string& track_name) const
{
}

std::pair<ext::ControlStatus, ext::TrackInfo> AudioGraphController::get_track_info(int track_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> AudioGraphController::get_track_processors(int track_id) const
{
}

std::pair<ext::ControlStatus, int> AudioGraphController::get_processor_id(const std::string& processor_name) const
{
}

std::pair<ext::ControlStatus, ext::ProcessorInfo> AudioGraphController::get_processor_info(int processor_id) const
{
}

std::pair<ext::ControlStatus, bool> AudioGraphController::get_processor_bypass_state(int processor_id) const
{
}

ext::ControlStatus AudioGraphController::set_processor_bypass_state(int processor_id, bool bypass_enabled)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::create_track(const std::string& name, int channels)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::create_multibus_track(const std::string& name,
                                                               int input_busses,
                                                               int output_channels)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::move_processor_on_track(int processor_id,
                                                                 int source_track_id,
                                                                 int dest_track_id,
                                                                 std::optional<int> before_processor)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::create_processor_on_track(const std::string& name,
                                                                   const std::string& uid,
                                                                   const std::string& file,
                                                                   ext::PluginType type,
                                                                   int track_id,
                                                                   std::optional<int> before_processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::delete_processor_from_track(int processor_id, int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioGraphController::delete_track(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


std::pair<ext::ControlStatus, int> ProgramController::get_processor_current_program(int processor_id) const
{
    return {ext::ControlStatus::UNSUPPORTED_OPERATION, 0};
}

std::pair<ext::ControlStatus, std::string> ProgramController::get_processor_current_program_name(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::string> ProgramController::get_processor_program_name(int processor_id,
                                                                                                 int program_id) const
{
}

std::pair<ext::ControlStatus, std::vector<std::string>> ProgramController::get_processor_programs(int processor_id) const
{
}

ext::ControlStatus ProgramController::set_processor_program(int processor_id, int program_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>>
ParameterController::get_processor_parameters(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> ParameterController::get_track_parameters(int processor_id) const
{
}

std::pair<ext::ControlStatus, int>
ParameterController::get_parameter_id(int processor_id, const std::string& parameter) const
{
}

std::pair<ext::ControlStatus, ext::ParameterInfo> ParameterController::get_parameter_info(int processor_id,
                                                                                          int parameter_id) const
{
}

std::pair<ext::ControlStatus, float> ParameterController::get_parameter_value(int processor_id,
                                                                              int parameter_id) const
{
}

std::pair<ext::ControlStatus, float> ParameterController::get_parameter_value_in_domain(int processor_id,
                                                                                        int parameter_id) const
{
}

std::pair<ext::ControlStatus, std::string> ParameterController::get_parameter_value_as_string(int processor_id,
                                                                                              int parameter_id) const
{
}

std::pair<ext::ControlStatus, std::string> ParameterController::get_string_property_value(int processor_id,
                                                                                          int parameter_id) const
{

}

ext::ControlStatus ParameterController::set_parameter_value(int processor_id,
                                                            int parameter_id,
                                                            float value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus ParameterController::set_string_property_value(int processor_id,
                                                                  int parameter_id,
                                                                  const std::string& value)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


int MidiController::get_input_ports() const
{
    return 0;
}

int MidiController::get_output_ports() const
{
    return 0;
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_input_connections() const
{
    return std::vector<ext::MidiKbdConnection>();
}

std::vector<ext::MidiKbdConnection> MidiController::get_all_kbd_output_connections() const
{
    return std::vector<ext::MidiKbdConnection>();
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_input_connections() const
{
}

std::vector<ext::MidiCCConnection> MidiController::get_all_cc_output_connections() const
{
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
MidiController::get_cc_input_connections_for_processor(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
MidiController::get_cc_output_connections_for_processor(int processor_id) const
{
}

ext::ControlStatus MidiController::connect_kbd_input_to_track(int track_id,
                                                              ext::MidiChannel channel,
                                                              int port,
                                                              bool raw_midi)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_kbd_output_from_track(int track_id,
                                                                 ext::MidiChannel channel,
                                                                 int port,
                                                                 bool raw_midi)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_cc_to_parameter(int processor_id,
                                                           ext::MidiChannel channel,
                                                           int port,
                                                           int cc_number,
                                                           int min_range,
                                                           int max_range,
                                                           bool relative_mode)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_pc(int processor_id, ext::MidiChannel channel, int port)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_all_cc_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus MidiController::disconnect_all_pc_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

std::vector<ext::AudioConnection> AudioRoutingController::get_all_input_connections() const
{
}

std::vector<ext::AudioConnection> AudioRoutingController::get_all_output_connections() const
{
}

std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>>
AudioRoutingController::get_input_connections_for_track(int track_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>>
AudioRoutingController::get_output_connections_for_track(int track_id) const
{
}

ext::ControlStatus
AudioRoutingController::connect_input_channel_to_track(int track_id, int track_channel, int input_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
AudioRoutingController::connect_output_channel_to_track(int track_id, int track_channel, int output_channel)
{
    return AudioRoutingController::connect_output_channel_to_track(track_id, track_channel, output_channel);
}

ext::ControlStatus AudioRoutingController::disconnect_input(int track_id, int track_channel, int input_channel)
{
    return AudioRoutingController::disconnect_input(track_id, track_channel, input_channel);
}

ext::ControlStatus AudioRoutingController::disconnect_output(int track_id, int track_channel, int output_channel)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_all_inputs_from_track(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus AudioRoutingController::disconnect_all_outputs_from_track(int track_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


int CvGateController::get_cv_input_ports() const
{
    return 0;
}

int CvGateController::get_cv_output_ports() const
{
    return 0;
}

std::vector<ext::CvConnection> CvGateController::get_all_cv_input_connections() const
{
}

std::vector<ext::CvConnection> CvGateController::get_all_cv_output_connections() const
{
}

std::vector<ext::GateConnection> CvGateController::get_all_gate_input_connections() const
{
}

std::vector<ext::GateConnection> CvGateController::get_all_gate_output_connections() const
{
}

std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
CvGateController::get_cv_input_connections_for_processor(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
CvGateController::get_cv_output_connections_for_processor(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
CvGateController::get_gate_input_connections_for_processor(int processor_id) const
{
}

std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
CvGateController::get_gate_output_connections_for_processor(int processor_id) const
{
}

ext::ControlStatus CvGateController::connect_cv_input_to_parameter(int processor_id, int parameter_id, int cv_input_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
CvGateController::connect_cv_output_from_parameter(int processor_id, int parameter_id, int cv_output_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
CvGateController::connect_gate_input_to_processor(int processor_id, int gate_input_id, int channel, int note_no)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
CvGateController::connect_gate_output_from_processor(int processor_id, int gate_output_id, int channel, int note_no)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_cv_input(int processor_id, int parameter_id, int cv_input_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_cv_output(int processor_id, int parameter_id, int cv_output_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
CvGateController::disconnect_gate_input(int processor_id, int gate_input_id, int channel, int note_no)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus
CvGateController::disconnect_gate_output(int processor_id, int gate_output_id, int channel, int note_no)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_all_cv_inputs_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_all_cv_outputs_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_all_gate_inputs_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus CvGateController::disconnect_all_gate_outputs_from_processor(int processor_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}


int OscController::get_send_port() const
{
    return 0;
}

int OscController::get_receive_port() const
{
    return 0;
}

std::vector<std::string> OscController::get_enabled_parameter_outputs() const
{
    return std::vector<std::string>();
}

ext::ControlStatus OscController::enable_output_for_parameter(int processor_id, int parameter_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}

ext::ControlStatus OscController::disable_output_for_parameter(int processor_id, int parameter_id)
{
    return ext::ControlStatus::UNSUPPORTED_OPERATION;
}
} // namespace controller_impl
} // namespace engine
} // namespace sushi