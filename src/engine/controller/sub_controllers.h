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

#ifndef SUSHI_CONTROLLER_IMPL_H
#define SUSHI_CONTROLLER_IMPL_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class SystemController : public ext::SystemController
{
public:
    SystemController(BaseEngine* engine) : _engine(engine) {}

    ~SystemController() = default;

    std::string get_interface_version() const override;

    std::string get_sushi_version() const override;

    ext::SushiBuildInfo get_sushi_buildinfo() const override;

    int get_input_audio_channel_count() const override;

    int get_output_audio_channel_count() const override;

private:
    BaseEngine* _engine;
};

class TransportController : public ext::TransportController
{
public:
    TransportController(BaseEngine* engine) : _engine(engine) {}

    ~TransportController() override = default;

    float get_samplerate() const override;

    ext::PlayingMode get_playing_mode() const override;

    ext::SyncMode get_sync_mode() const override;

    ext::TimeSignature get_time_signature() const override;

    float get_tempo() const override;

    void set_sync_mode(ext::SyncMode sync_mode) override;

    void set_playing_mode(ext::PlayingMode playing_mode) override;

    ext::ControlStatus set_tempo(float tempo) override;

    ext::ControlStatus set_time_signature(ext::TimeSignature signature) override;

private:
    BaseEngine* _engine;
};

class TimingController : public ext::TimingController
{
public:
    TimingController(BaseEngine* engine) : _engine(engine) {}

    ~TimingController() override = default;

    bool get_timing_statistics_enabled() const override;

    void set_timing_statistics_enabled(bool enabled) override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_engine_timings() const override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_track_timings(int track_id) const override;

    std::pair<ext::ControlStatus, ext::CpuTimings> get_processor_timings(int processor_id) const override;

    ext::ControlStatus reset_all_timings() override;

    ext::ControlStatus reset_track_timings(int track_id) override;

    ext::ControlStatus reset_processor_timings(int processor_id) override;

private:
    BaseEngine* _engine;
};

class KeyboardController : public ext::KeyboardController
{
public:
    KeyboardController(BaseEngine* engine) : _engine(engine) {}

    ~KeyboardController() override = default;

    ext::ControlStatus send_note_on(int track_id, int channel, int note, float velocity) override;

    ext::ControlStatus send_note_off(int track_id, int channel, int note, float velocity) override;

    ext::ControlStatus send_note_aftertouch(int track_id, int channel, int note, float value) override;

    ext::ControlStatus send_aftertouch(int track_id, int channel, float value) override;

    ext::ControlStatus send_pitch_bend(int track_id, int channel, float value) override;

    ext::ControlStatus send_modulation(int track_id, int channel, float value) override;

private:
    BaseEngine* _engine;
};

class AudioGraphController : public ext::AudioGraphController
{
public:
    AudioGraphController(BaseEngine* engine) : _engine(engine) {}

    ~AudioGraphController() override = default;

    std::vector<ext::ProcessorInfo> get_all_processors() const override;

    std::vector<ext::TrackInfo> get_all_tracks() const override;

    std::pair<ext::ControlStatus, int> get_track_id(const std::string& track_name) const override;

    std::pair<ext::ControlStatus, ext::TrackInfo> get_track_info(int track_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::ProcessorInfo>> get_track_processors(int track_id) const override;

    std::pair<ext::ControlStatus, int> get_processor_id(const std::string& processor_name) const override;

    std::pair<ext::ControlStatus, ext::ProcessorInfo> get_processor_info(int processor_id) const override;

    std::pair<ext::ControlStatus, bool> get_processor_bypass_state(int processor_id) const override;

    ext::ControlStatus set_processor_bypass_state(int processor_id, bool bypass_enabled) override;

    ext::ControlStatus create_track(const std::string& name, int channels) override;

    ext::ControlStatus create_multibus_track(const std::string& name, int input_busses, int output_channels) override;

    ext::ControlStatus move_processor_on_track(int processor_id,
                                               int source_track_id,
                                               int dest_track_id,
                                               std::optional<int> before_processor) override;

    ext::ControlStatus create_processor_on_track(const std::string& name,
                                                 const std::string& uid,
                                                 const std::string& file,
                                                 ext::PluginType type,
                                                 int track_id,
                                                 std::optional<int> before_processor_id) override;

    ext::ControlStatus delete_processor_from_track(int processor_id, int track_id) override;

    ext::ControlStatus delete_track(int track_id) override;

private:
    BaseEngine* _engine;
};

class ProgramController : public ext::ProgramController
{
public:
    ProgramController(BaseEngine* engine) : _engine(engine) {}

    ~ProgramController() override = default;

    std::pair<ext::ControlStatus, int> get_processor_current_program(int processor_id) const override;

    std::pair<ext::ControlStatus, std::string> get_processor_current_program_name(int processor_id) const override;

    std::pair<ext::ControlStatus, std::string> get_processor_program_name(int processor_id, int program_id) const override;

    std::pair<ext::ControlStatus, std::vector<std::string>> get_processor_programs(int processor_id) const override;

    ext::ControlStatus set_processor_program(int processor_id, int program_id) override;

private:
    BaseEngine* _engine;
};

class ParameterController : public ext::ParameterController
{
public:
    ParameterController(BaseEngine* engine) : _engine(engine) {}

    ~ParameterController() override = default;

    std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> get_processor_parameters(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::ParameterInfo>> get_track_parameters(int processor_id) const override;

    std::pair<ext::ControlStatus, int> get_parameter_id(int processor_id, const std::string& parameter) const override;

    std::pair<ext::ControlStatus, ext::ParameterInfo> get_parameter_info(int processor_id, int parameter_id) const override;

    std::pair<ext::ControlStatus, float> get_parameter_value(int processor_id, int parameter_id) const override;

    std::pair<ext::ControlStatus, float> get_parameter_value_in_domain(int processor_id, int parameter_id) const override;

    std::pair<ext::ControlStatus, std::string>
    get_parameter_value_as_string(int processor_id, int parameter_id) const override;

    std::pair<ext::ControlStatus, std::string> get_string_property_value(int processor_id, int parameter_id) const override;

    ext::ControlStatus set_parameter_value(int processor_id, int parameter_id, float value) override;

    ext::ControlStatus set_string_property_value(int processor_id, int parameter_id, const std::string& value) override;

private:
    BaseEngine* _engine;
};

class MidiController : public ext::MidiController
{
public:
    MidiController(BaseEngine* engine) : _engine(engine) {}

    ~MidiController() override = default;

    int get_input_ports() const override;

    int get_output_ports() const override;

    std::vector<ext::MidiKbdConnection> get_all_kbd_input_connections() const override;

    std::vector<ext::MidiKbdConnection> get_all_kbd_output_connections() const override;

    std::vector<ext::MidiCCConnection> get_all_cc_input_connections() const override;

    std::vector<ext::MidiCCConnection> get_all_cc_output_connections() const override;

    std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
    get_cc_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::MidiCCConnection>>
    get_cc_output_connections_for_processor(int processor_id) const override;

    ext::ControlStatus
    connect_kbd_input_to_track(int track_id, ext::MidiChannel channel, int port, bool raw_midi) override;

    ext::ControlStatus
    connect_kbd_output_from_track(int track_id, ext::MidiChannel channel, int port, bool raw_midi) override;

    ext::ControlStatus connect_cc_to_parameter(int processor_id,
                                               ext::MidiChannel channel,
                                               int port,
                                               int cc_number,
                                               int min_range,
                                               int max_range,
                                               bool relative_mode) override;

    ext::ControlStatus connect_pc_to_processor(int processor_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_kbd_input(int track_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_kbd_output(int track_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_cc(int processor_id, ext::MidiChannel channel, int port, int cc_number) override;

    ext::ControlStatus disconnect_pc(int processor_id, ext::MidiChannel channel, int port) override;

    ext::ControlStatus disconnect_all_cc_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_pc_from_processor(int processor_id) override;

private:
    BaseEngine* _engine;
};

class AudioRoutingController : public ext::AudioRoutingController
{
public:
    AudioRoutingController(BaseEngine* engine) : _engine(engine) {}

    ~AudioRoutingController() override = default;

    std::vector<ext::AudioConnection> get_all_input_connections() const override;

    std::vector<ext::AudioConnection> get_all_output_connections() const override;

    std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>> get_input_connections_for_track(int track_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::AudioConnection>>
    get_output_connections_for_track(int track_id) const override;

    ext::ControlStatus connect_input_channel_to_track(int track_id, int track_channel, int input_channel) override;

    ext::ControlStatus connect_output_channel_to_track(int track_id, int track_channel, int output_channel) override;

    ext::ControlStatus disconnect_input(int track_id, int track_channel, int input_channel) override;

    ext::ControlStatus disconnect_output(int track_id, int track_channel, int output_channel) override;

    ext::ControlStatus disconnect_all_inputs_from_track(int track_id) override;

    ext::ControlStatus disconnect_all_outputs_from_track(int track_id) override;

private:
    BaseEngine* _engine;
};

class CvGateController : public ext::CvGateController
{
public:
    CvGateController(BaseEngine* engine) : _engine(engine) {}

    ~CvGateController() override = default;

    int get_cv_input_ports() const override;

    int get_cv_output_ports() const override;

    std::vector<ext::CvConnection> get_all_cv_input_connections() const override;

    std::vector<ext::CvConnection> get_all_cv_output_connections() const override;

    std::vector<ext::GateConnection> get_all_gate_input_connections() const override;

    std::vector<ext::GateConnection> get_all_gate_output_connections() const override;

    std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
    get_cv_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
    get_cv_output_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
    get_gate_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
    get_gate_output_connections_for_processor(int processor_id) const override;

    ext::ControlStatus connect_cv_input_to_parameter(int processor_id, int parameter_id, int cv_input_id) override;

    ext::ControlStatus connect_cv_output_from_parameter(int processor_id, int parameter_id, int cv_output_id) override;

    ext::ControlStatus
    connect_gate_input_to_processor(int processor_id, int gate_input_id, int channel, int note_no) override;

    ext::ControlStatus
    connect_gate_output_from_processor(int processor_id, int gate_output_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_cv_input(int processor_id, int parameter_id, int cv_input_id) override;

    ext::ControlStatus disconnect_cv_output(int processor_id, int parameter_id, int cv_output_id) override;

    ext::ControlStatus disconnect_gate_input(int processor_id, int gate_input_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_gate_output(int processor_id, int gate_output_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_all_cv_inputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_cv_outputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_gate_inputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_gate_outputs_from_processor(int processor_id) override;

private:
    BaseEngine* _engine;
};

class OscController : public ext::OscController
{
public:
    OscController(BaseEngine* engine) : _engine(engine) {}

    ~OscController() override = default;

    int get_send_port() const override;

    int get_receive_port() const override;

    std::vector<std::string> get_enabled_parameter_outputs() const override;

    ext::ControlStatus enable_output_for_parameter(int processor_id, int parameter_id) override;

    ext::ControlStatus disable_output_for_parameter(int processor_id, int parameter_id) override;

private:
    BaseEngine* _engine;
};


} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_CONTROLLER_IMPL_H
