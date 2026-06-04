/*
* Copyright 2017-2026 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Sushi Control Service, zmq service for ipc control of Sushi
 * @Copyright 2017-2026 Elk Audio AB, Stockholm
 */

#include <elklog/static_logger.h>

#include "zmq_control_service.h"
#include "conversions.h"


ELKLOG_GET_LOGGER_WITH_MODULE_NAME("zmq_controller");

namespace sushi_ipc {

using namespace sushi_rpc;

//=============================================================================
// SystemControlService
//=============================================================================

void SystemControlService::GetSushiVersion(const sushi_rpc::GenericVoidValue& /*request*/,
                                            sushi_rpc::GenericStringValue& response)
{
    response.set_value(_controller->get_sushi_version());
}

void SystemControlService::GetSushiApiVersion(const sushi_rpc::GenericVoidValue& /*request*/,
                                               sushi_rpc::GenericStringValue& response)
{
    response.set_value(_controller->get_sushi_api_version());
}

void SystemControlService::GetBuildInfo(const sushi_rpc::GenericVoidValue& /*request*/,
                                        sushi_rpc::SushiBuildInfo& response)
{
    auto build_info = _controller->get_sushi_build_info();
    to_grpc(response, build_info);
}

void SystemControlService::GetInputAudioChannelCount(const sushi_rpc::GenericVoidValue& /*request*/,
                                                     sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_input_audio_channel_count());
}

void SystemControlService::GetOutputAudioChannelCount(const sushi_rpc::GenericVoidValue& /*request*/,
                                                      sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_output_audio_channel_count());
}

//=============================================================================
// TransportControlService
//=============================================================================

void TransportControlService::GetSamplerate(const sushi_rpc::GenericVoidValue& /*request*/,
                                            sushi_rpc::GenericFloatValue& response)
{
    response.set_value(_controller->get_samplerate());
}

void TransportControlService::GetPlayingMode(const sushi_rpc::GenericVoidValue& /*request*/,
                                             sushi_rpc::PlayingMode& response)
{
    response.set_mode(to_grpc(_controller->get_playing_mode()));
}

void TransportControlService::GetSyncMode(const sushi_rpc::GenericVoidValue& /*request*/,
                                          sushi_rpc::SyncMode& response)
{
    response.set_mode(to_grpc(_controller->get_sync_mode()));
}

void TransportControlService::GetTimeSignature(const sushi_rpc::GenericVoidValue& /*request*/,
                                               sushi_rpc::TimeSignature& response)
{
    auto ts = _controller->get_time_signature();
    response.set_denominator(ts.denominator);
    response.set_numerator(ts.numerator);
}

void TransportControlService::GetTempo(const sushi_rpc::GenericVoidValue& /*request*/,
                                       sushi_rpc::GenericFloatValue& response)
{
    response.set_value(_controller->get_tempo());
}

void TransportControlService::SetTempo(const sushi_rpc::GenericFloatValue& request,
                                       sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_tempo(request.value());
    to_grpc(response, status);
}

void TransportControlService::SetPlayingMode(const sushi_rpc::PlayingMode& request,
                                             sushi_rpc::CommandResponse& response)
{
    _controller->set_playing_mode(to_sushi_ext(request.mode()));
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::SUCCESS);
}

void TransportControlService::SetSyncMode(const sushi_rpc::SyncMode& request,
                                          sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_sync_mode(to_sushi_ext(request.mode()));
    to_grpc(response, status);
}

void TransportControlService::SetTimeSignature(const sushi_rpc::TimeSignature& request,
                                               sushi_rpc::CommandResponse& response)
{
    sushi::control::TimeSignature ts;
    ts.numerator = request.numerator();
    ts.denominator = request.denominator();
    auto status = _controller->set_time_signature(ts);
    to_grpc(response, status);
}

//=============================================================================
// TimingControlService
//=============================================================================

void TimingControlService::GetTimingsEnabled(const sushi_rpc::GenericVoidValue& /*request*/,
                                             sushi_rpc::GenericBoolValue& response)
{
    response.set_value(_controller->get_timing_statistics_enabled());
}

void TimingControlService::SetTimingsEnabled(const sushi_rpc::GenericBoolValue& request,
                                             sushi_rpc::CommandResponse& response)
{
    _controller->set_timing_statistics_enabled(request.value());
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::SUCCESS);
}

void TimingControlService::GetEngineTimings(const sushi_rpc::GenericVoidValue& /*request*/,
                                            sushi_rpc::CpuTimings& response)
{
    auto [status, timings] = _controller->get_engine_timings();
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(response, timings);
    }
}

void TimingControlService::GetTrackTimings(const sushi_rpc::TrackIdentifier& request,
                                           sushi_rpc::TimingResponse& response)
{
    auto [status, timings] = _controller->get_track_timings(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_timings(), timings);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void TimingControlService::GetProcessorTimings(const sushi_rpc::ProcessorIdentifier& request,
                                               sushi_rpc::TimingResponse& response)
{
    auto [status, timings] = _controller->get_processor_timings(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_timings(), timings);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void TimingControlService::ResetAllTimings(const sushi_rpc::GenericVoidValue& /*request*/,
                                           sushi_rpc::CommandResponse& response)
{
    _controller->reset_all_timings();
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::SUCCESS);
}

void TimingControlService::ResetTrackTimings(const sushi_rpc::TrackIdentifier& request,
                                             sushi_rpc::CommandResponse& response)
{
    auto status = _controller->reset_track_timings(request.id());
    to_grpc(response, status);
}

void TimingControlService::ResetProcessorTimings(const sushi_rpc::ProcessorIdentifier& request,
                                                 sushi_rpc::CommandResponse& response)
{
    auto status = _controller->reset_processor_timings(request.id());
    to_grpc(response, status);
}

//=============================================================================
// KeyboardControlService
//=============================================================================

void KeyboardControlService::SendNoteOn(const sushi_rpc::NoteOnRequest& request,
                                        sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_note_on(request.track().id(), request.channel(), request.note(), request.velocity());
    to_grpc(response, status);
}

void KeyboardControlService::SendNoteOff(const sushi_rpc::NoteOffRequest& request,
                                         sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_note_off(request.track().id(), request.channel(), request.note(), request.velocity());
    to_grpc(response, status);
}

void KeyboardControlService::SendNoteAftertouch(const sushi_rpc::NoteAftertouchRequest& request,
                                                sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_note_aftertouch(request.track().id(), request.channel(), request.note(), request.value());
    to_grpc(response, status);
}

void KeyboardControlService::SendAftertouch(const sushi_rpc::NoteModulationRequest& request,
                                            sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_aftertouch(request.track().id(), request.channel(), request.value());
    to_grpc(response, status);
}

void KeyboardControlService::SendPitchBend(const sushi_rpc::NoteModulationRequest& request,
                                           sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_pitch_bend(request.track().id(), request.channel(), request.value());
    to_grpc(response, status);
}

void KeyboardControlService::SendModulation(const sushi_rpc::NoteModulationRequest& request,
                                            sushi_rpc::CommandResponse& response)
{
    auto status = _controller->send_modulation(request.track().id(), request.channel(), request.value());
    to_grpc(response, status);
}

//=============================================================================
// AudioGraphControlService
//=============================================================================

void AudioGraphControlService::GetAllProcessors(const sushi_rpc::GenericVoidValue& /*request*/,
                                                sushi_rpc::ProcessorInfoList& response)
{
    auto processors = _controller->get_all_processors();
    for (const auto& processor : processors)
    {
        to_grpc(*response.add_processors(), processor);
    }
}

void AudioGraphControlService::GetAllTracks(const sushi_rpc::GenericVoidValue& /*request*/,
                                            sushi_rpc::TrackInfoList& response)
{
    auto tracks = _controller->get_all_tracks();
    for (const auto& track : tracks)
    {
        to_grpc(*response.add_tracks(), track);
    }
}

void AudioGraphControlService::GetTrackId(const sushi_rpc::GenericStringValue& request,
                                          sushi_rpc::TrackIdentifierResponse& response)
{
    auto [status, id] = _controller->get_track_id(request.value());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_id(id);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetTrackInfo(const sushi_rpc::TrackIdentifier& request,
                                            sushi_rpc::TrackInfoResponse& response)
{
    auto [status, track] = _controller->get_track_info(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_info(), track);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetTrackProcessors(const sushi_rpc::TrackIdentifier& request,
                                                  sushi_rpc::ProcessorInfoListResponse& response)
{
    auto [status, processors] = _controller->get_track_processors(request.id());
    for (const auto& processor : processors)
    {
        to_grpc(*response.add_processors(), processor);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetProcessorId(const sushi_rpc::GenericStringValue& request,
                                              sushi_rpc::ProcessorIdentifierResponse& response)
{
    auto [status, id] = _controller->get_processor_id(request.value());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_id(id);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetProcessorInfo(const sushi_rpc::ProcessorIdentifier& request,
                                                sushi_rpc::ProcessorInfoResponse& response)
{
    auto [status, processor] = _controller->get_processor_info(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_processor(), processor);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetProcessorBypassState(const sushi_rpc::ProcessorIdentifier& request,
                                                       sushi_rpc::BoolResponse& response)
{
    auto [status, state] = _controller->get_processor_bypass_state(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(state);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::GetProcessorState(const sushi_rpc::ProcessorIdentifier& request,
                                                 sushi_rpc::ProcessorStateResponse& response)
{
    auto [status, state] = _controller->get_processor_state(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_state(), state);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioGraphControlService::SetProcessorBypassState(const sushi_rpc::ProcessorBypassStateSetRequest& request,
                                                       sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_processor_bypass_state(request.processor().id(), request.value());
    to_grpc(response, status);
}

void AudioGraphControlService::SetProcessorState(const sushi_rpc::ProcessorStateSetRequest& request,
                                                 sushi_rpc::CommandResponse& response)
{
    sushi::control::ProcessorState sushi_state;
    to_sushi_ext(sushi_state, request.state());
    auto status = _controller->set_processor_state(request.processor().id(), sushi_state);
    to_grpc(response, status);
}

void AudioGraphControlService::CreateTrack(const sushi_rpc::CreateTrackRequest& request,
                                           sushi_rpc::CommandResponse& response)
{
    auto thread = (request.thread().has_value() ? std::optional<int>(request.thread().value()) : std::nullopt);
    auto status = _controller->create_track(request.name(), request.channels(), thread);
    to_grpc(response, status);
}

void AudioGraphControlService::CreateMultibusTrack(const sushi_rpc::CreateMultibusTrackRequest& request,
                                                   sushi_rpc::CommandResponse& response)
{
    auto thread = (request.thread().has_value() ? std::optional<int>(request.thread().value()) : std::nullopt);
    auto status = _controller->create_multibus_track(request.name(), request.buses(), thread);
    to_grpc(response, status);
}

void AudioGraphControlService::CreatePreTrack(const sushi_rpc::CreatePreTrackRequest& request,
                                              sushi_rpc::CommandResponse& response)
{
    auto status = _controller->create_pre_track(request.name());
    to_grpc(response, status);
}

void AudioGraphControlService::CreatePostTrack(const sushi_rpc::CreatePostTrackRequest& request,
                                               sushi_rpc::CommandResponse& response)
{
    auto status = _controller->create_post_track(request.name());
    to_grpc(response, status);
}

void AudioGraphControlService::CreateProcessorOnTrack(const sushi_rpc::CreateProcessorRequest& request,
                                                      sushi_rpc::CommandResponse& response)
{
    std::optional<int> before_processor = std::nullopt;
    if (request.position().add_to_back() == false)
    {
        before_processor = request.position().before_processor().id();
    }
    auto status = _controller->create_processor_on_track(request.name(),
                                                         request.uid(),
                                                         request.path(),
                                                         to_sushi_ext(request.type().type()),
                                                         request.track().id(),
                                                         before_processor);
    to_grpc(response, status);
}

void AudioGraphControlService::MoveProcessorOnTrack(const sushi_rpc::MoveProcessorRequest& request,
                                                    sushi_rpc::CommandResponse& response)
{
    std::optional<int> before_processor = std::nullopt;
    if (request.position().add_to_back() == false)
    {
        before_processor = request.position().before_processor().id();
    }
    auto status = _controller->move_processor_on_track(request.processor().id(),
                                                       request.source_track().id(),
                                                       request.dest_track().id(),
                                                       before_processor);
    to_grpc(response, status);
}

void AudioGraphControlService::DeleteProcessorFromTrack(const sushi_rpc::DeleteProcessorRequest& request,
                                                        sushi_rpc::CommandResponse& response)
{
    auto status = _controller->delete_processor_from_track(request.processor().id(), request.track().id());
    to_grpc(response, status);
}

void AudioGraphControlService::DeleteTrack(const sushi_rpc::TrackIdentifier& request,
                                           sushi_rpc::CommandResponse& response)
{
    auto status = _controller->delete_track(request.id());
    to_grpc(response, status);
}

//=============================================================================
// ProgramControlService
//=============================================================================

void ProgramControlService::GetProcessorCurrentProgram(const sushi_rpc::ProcessorIdentifier& request,
                                                       sushi_rpc::ProgramIdentifierResponse& response)
{
    auto [status, program] = _controller->get_processor_current_program(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_program(program);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ProgramControlService::GetProcessorCurrentProgramName(const sushi_rpc::ProcessorIdentifier& request,
                                                           sushi_rpc::StringResponse& response)
{
    auto [status, program] = _controller->get_processor_current_program_name(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(program);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ProgramControlService::GetProcessorProgramName(const sushi_rpc::ProcessorProgramIdentifier& request,
                                                    sushi_rpc::StringResponse& response)
{
    auto [status, program] = _controller->get_processor_program_name(request.processor().id(), request.program());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(program);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ProgramControlService::GetProcessorPrograms(const sushi_rpc::ProcessorIdentifier& request,
                                                 sushi_rpc::ProgramInfoListResponse& response)
{
    auto [status, programs] = _controller->get_processor_programs(request.id());
    int id = 0;
    for (auto& program : programs)
    {
        auto info = response.add_programs();
        info->set_name(program);
        info->mutable_id()->set_program(id++);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ProgramControlService::SetProcessorProgram(const sushi_rpc::ProcessorProgramSetRequest& request,
                                                sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_processor_program(request.processor().id(), request.program().program());
    to_grpc(response, status);
}

//=============================================================================
// ParameterControlService
//=============================================================================

void ParameterControlService::GetTrackParameters(const sushi_rpc::TrackIdentifier& request,
                                                 sushi_rpc::ParameterInfoListResponse& response)
{
    auto [status, parameters] = _controller->get_track_parameters(request.id());
    for (const auto& parameter : parameters)
    {
        to_grpc(*response.add_parameters(), parameter);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetProcessorParameters(const sushi_rpc::ProcessorIdentifier& request,
                                                     sushi_rpc::ParameterInfoListResponse& response)
{
    auto [status, parameters] = _controller->get_processor_parameters(request.id());
    for (const auto& parameter : parameters)
    {
        to_grpc(*response.add_parameters(), parameter);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetParameterId(const sushi_rpc::ParameterIdRequest& request,
                                             sushi_rpc::ParameterIdentifierResponse& response)
{
    auto [status, id] = _controller->get_parameter_id(request.processor().id(), request.parametername());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.mutable_id()->set_parameter_id(id);
        response.mutable_id()->set_processor_id(request.processor().id());
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetParameterInfo(const sushi_rpc::ParameterIdentifier& request,
                                               sushi_rpc::ParameterInfoResponse& response)
{
    auto [status, parameter] = _controller->get_parameter_info(request.processor_id(), request.parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_info(), parameter);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetParameterValue(const sushi_rpc::ParameterIdentifier& request,
                                                sushi_rpc::FloatResponse& response)
{
    auto [status, value] = _controller->get_parameter_value(request.processor_id(), request.parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(value);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetParameterValueInDomain(const sushi_rpc::ParameterIdentifier& request,
                                                        sushi_rpc::FloatResponse& response)
{
    auto [status, value] = _controller->get_parameter_value_in_domain(request.processor_id(), request.parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(value);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetParameterValueAsString(const sushi_rpc::ParameterIdentifier& request,
                                                        sushi_rpc::StringResponse& response)
{
    auto [status, value] = _controller->get_parameter_value_as_string(request.processor_id(), request.parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(value);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::SetParameterValue(const sushi_rpc::ParameterValue& request,
                                                sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_parameter_value(request.parameter().processor_id(),
                                                   request.parameter().parameter_id(),
                                                   request.value());
    to_grpc(response, status);
}

void ParameterControlService::GetTrackProperties(const sushi_rpc::TrackIdentifier& request,
                                                 sushi_rpc::PropertyInfoListResponse& response)
{
    auto [status, properties] = _controller->get_track_properties(request.id());
    for (const auto& property : properties)
    {
        to_grpc(*response.add_properties(), property);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetProcessorProperties(const sushi_rpc::ProcessorIdentifier& request,
                                                     sushi_rpc::PropertyInfoListResponse& response)
{
    auto [status, properties] = _controller->get_processor_properties(request.id());
    for (const auto& property : properties)
    {
        to_grpc(*response.add_properties(), property);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetPropertyId(const sushi_rpc::PropertyIdRequest& request,
                                            sushi_rpc::PropertyIdentifierResponse& response)
{
    auto [status, id] = _controller->get_property_id(request.processor().id(), request.property_name());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.mutable_id()->set_property_id(id);
        response.mutable_id()->set_processor_id(request.processor().id());
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetPropertyInfo(const sushi_rpc::PropertyIdentifier& request,
                                              sushi_rpc::PropertyInfoResponse& response)
{
    auto [status, property] = _controller->get_property_info(request.processor_id(), request.property_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_grpc(*response.mutable_info(), property);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::GetPropertyValue(const sushi_rpc::PropertyIdentifier& request,
                                               sushi_rpc::StringResponse& response)
{
    auto [status, value] = _controller->get_property_value(request.processor_id(), request.property_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response.set_value(std::move(value));
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void ParameterControlService::SetPropertyValue(const sushi_rpc::PropertyValue& request,
                                               sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_property_value(request.property().processor_id(),
                                                  request.property().property_id(),
                                                  request.value());
    to_grpc(response, status);
}

//=============================================================================
// MidiControlService
//=============================================================================

void MidiControlService::GetInputPorts(const sushi_rpc::GenericVoidValue& /*request*/,
                                       sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_input_ports());
}

void MidiControlService::GetOutputPorts(const sushi_rpc::GenericVoidValue& /*request*/,
                                        sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_output_ports());
}

void MidiControlService::GetAllKbdInputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                   sushi_rpc::MidiKbdConnectionList& response)
{
    auto connections = _controller->get_all_kbd_input_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void MidiControlService::GetAllKbdOutputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                    sushi_rpc::MidiKbdConnectionList& response)
{
    auto connections = _controller->get_all_kbd_output_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void MidiControlService::GetAllCCInputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                  sushi_rpc::MidiCCConnectionList& response)
{
    auto connections = _controller->get_all_cc_input_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void MidiControlService::GetAllPCInputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                  sushi_rpc::MidiPCConnectionList& response)
{
    auto connections = _controller->get_all_pc_input_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void MidiControlService::GetCCInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request,
                                                           sushi_rpc::MidiCCConnectionListResponse& response)
{
    auto [status, connections] = _controller->get_cc_input_connections_for_processor(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        for (const auto& connection : connections)
        {
            to_grpc(*response.add_connections(), connection);
        }
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void MidiControlService::GetPCInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request,
                                                           sushi_rpc::MidiPCConnectionListResponse& response)
{
    auto [status, connections] = _controller->get_pc_input_connections_for_processor(request.id());
    if (status == sushi::control::ControlStatus::OK)
    {
        for (const auto& connection : connections)
        {
            to_grpc(*response.add_connections(), connection);
        }
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void MidiControlService::GetMidiClockOutputEnabled(const sushi_rpc::GenericIntValue& request,
                                                   sushi_rpc::GenericBoolValue& response)
{
    response.set_value(_controller->get_midi_clock_output_enabled(request.value()));
}

void MidiControlService::SetMidiClockOutputEnabled(const sushi_rpc::MidiClockSetRequest& request,
                                                   sushi_rpc::CommandResponse& response)
{
    auto status = _controller->set_midi_clock_output_enabled(request.enabled(), request.port());
    to_grpc(response, status);
}

void MidiControlService::ConnectKbdInputToTrack(const sushi_rpc::MidiKbdConnection& request,
                                                sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->connect_kbd_input_to_track(request.track().id(), midi_channel, request.port(), request.raw_midi());
    to_grpc(response, status);
}

void MidiControlService::ConnectKbdOutputFromTrack(const sushi_rpc::MidiKbdConnection& request,
                                                   sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->connect_kbd_output_from_track(request.track().id(), midi_channel, request.port());
    to_grpc(response, status);
}

void MidiControlService::ConnectCCToParameter(const sushi_rpc::MidiCCConnection& request,
                                              sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->connect_cc_to_parameter(request.parameter().processor_id(),
                                                       request.parameter().parameter_id(),
                                                       midi_channel,
                                                       request.port(),
                                                       request.cc_number(),
                                                       request.min_range(),
                                                       request.max_range(),
                                                       request.relative_mode());
    to_grpc(response, status);
}

void MidiControlService::ConnectPCToProcessor(const sushi_rpc::MidiPCConnection& request,
                                              sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->connect_pc_to_processor(request.processor().id(), midi_channel, request.port());
    to_grpc(response, status);
}

void MidiControlService::DisconnectKbdInput(const sushi_rpc::MidiKbdConnection& request,
                                            sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->disconnect_kbd_input(request.track().id(), midi_channel, request.port(), request.raw_midi());
    to_grpc(response, status);
}

void MidiControlService::DisconnectKbdOutput(const sushi_rpc::MidiKbdConnection& request,
                                             sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->disconnect_kbd_output(request.track().id(), midi_channel, request.port());
    to_grpc(response, status);
}

void MidiControlService::DisconnectCC(const sushi_rpc::MidiCCConnection& request,
                                      sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->disconnect_cc(request.parameter().processor_id(),
                                             midi_channel,
                                             request.port(),
                                             request.cc_number());
    to_grpc(response, status);
}

void MidiControlService::DisconnectPC(const sushi_rpc::MidiPCConnection& request,
                                      sushi_rpc::CommandResponse& response)
{
    const auto midi_channel = to_sushi_ext(request.channel().channel());
    auto status = _controller->disconnect_pc(request.processor().id(), midi_channel, request.port());
    to_grpc(response, status);
}

void MidiControlService::DisconnectAllCCFromProcessor(const sushi_rpc::ProcessorIdentifier& request,
                                                      sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_all_cc_from_processor(request.id());
    to_grpc(response, status);
}

void MidiControlService::DisconnectAllPCFromProcessor(const sushi_rpc::ProcessorIdentifier& request,
                                                      sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_all_pc_from_processor(request.id());
    to_grpc(response, status);
}

//=============================================================================
// AudioRoutingControlService
//=============================================================================

void AudioRoutingControlService::GetAllInputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                        sushi_rpc::AudioConnectionList& response)
{
    auto connections = _controller->get_all_input_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void AudioRoutingControlService::GetAllOutputConnections(const sushi_rpc::GenericVoidValue& /*request*/,
                                                         sushi_rpc::AudioConnectionList& response)
{
    auto connections = _controller->get_all_output_connections();
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
}

void AudioRoutingControlService::GetInputConnectionsForTrack(const sushi_rpc::TrackIdentifier& request,
                                                             sushi_rpc::AudioConnectionListResponse& response)
{
    auto [status, connections] = _controller->get_input_connections_for_track(request.id());
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioRoutingControlService::GetOutputConnectionsForTrack(const sushi_rpc::TrackIdentifier& request,
                                                              sushi_rpc::AudioConnectionListResponse& response)
{
    auto [status, connections] = _controller->get_output_connections_for_track(request.id());
    for (const auto& connection : connections)
    {
        to_grpc(*response.add_connections(), connection);
    }
    response.mutable_status()->set_status(to_grpc(status));
}

void AudioRoutingControlService::ConnectInputChannelToTrack(const sushi_rpc::AudioConnection& request,
                                                            sushi_rpc::CommandResponse& response)
{
    auto status = _controller->connect_input_channel_to_track(request.track().id(),
                                                              request.track_channel(),
                                                              request.engine_channel());
    to_grpc(response, status);
}

void AudioRoutingControlService::ConnectOutputChannelFromTrack(const sushi_rpc::AudioConnection& request,
                                                               sushi_rpc::CommandResponse& response)
{
    auto status = _controller->connect_output_channel_to_track(request.track().id(),
                                                               request.track_channel(),
                                                               request.engine_channel());
    to_grpc(response, status);
}

void AudioRoutingControlService::DisconnectInput(const sushi_rpc::AudioConnection& request,
                                                 sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_input(request.track().id(),
                                                request.track_channel(),
                                                request.engine_channel());
    to_grpc(response, status);
}

void AudioRoutingControlService::DisconnectOutput(const sushi_rpc::AudioConnection& request,
                                                  sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_output(request.track().id(),
                                                 request.track_channel(),
                                                 request.engine_channel());
    to_grpc(response, status);
}

void AudioRoutingControlService::DisconnectAllInputsFromTrack(const sushi_rpc::TrackIdentifier& request,
                                                              sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_all_inputs_from_track(request.id());
    to_grpc(response, status);
}

void AudioRoutingControlService::DisconnectAllOutputFromTrack(const sushi_rpc::TrackIdentifier& /*request*/,
                                                              sushi_rpc::GenericVoidValue& /*response*/)
{
    // Deprecated — use DisconnectAllOutputsFromTrack
}

void AudioRoutingControlService::DisconnectAllOutputsFromTrack(const sushi_rpc::TrackIdentifier& request,
                                                               sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disconnect_all_outputs_from_track(request.id());
    to_grpc(response, status);
}

//=============================================================================
// CvGateControlService — empty stubs
//=============================================================================

void CvGateControlService::GetCvInputChannelCount(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::GenericIntValue& /*response*/) {}
void CvGateControlService::GetCvOutputChannelCount(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::GenericIntValue& /*response*/) {}
void CvGateControlService::GetAllCvInputConnections(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::CvConnectionList& /*response*/) {}
void CvGateControlService::GetAllCvOutputConnections(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::CvConnectionList& /*response*/) {}
void CvGateControlService::GetAllGateInputConnections(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::GateConnectionList& /*response*/) {}
void CvGateControlService::GetAllGateOutputConnections(const sushi_rpc::GenericVoidValue& /*request*/, sushi_rpc::GateConnectionList& /*response*/) {}
void CvGateControlService::GetCvInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CvConnectionListResponse& /*response*/) {}
void CvGateControlService::GetCvOutputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CvConnectionListResponse& /*response*/) {}
void CvGateControlService::GetGateInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::GateConnectionListResponse& /*response*/) {}
void CvGateControlService::GetGateOutputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::GateConnectionListResponse& /*response*/) {}
void CvGateControlService::ConnectCvInputToParameter(const sushi_rpc::CvConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::ConnectCvOutputFromParameter(const sushi_rpc::CvConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::ConnectGateInputToProcessor(const sushi_rpc::GateConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::ConnectGateOutputFromProcessor(const sushi_rpc::GateConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectCvInput(const sushi_rpc::CvConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectCvOutput(const sushi_rpc::CvConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectGateInput(const sushi_rpc::GateConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectGateOutput(const sushi_rpc::GateConnection& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectAllCvInputsFromProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectAllCvOutputsFromProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectAllGateInputsFromProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

void CvGateControlService::DisconnectAllGateOutputsFromProcessor(const sushi_rpc::ProcessorIdentifier& /*request*/, sushi_rpc::CommandResponse& response)
{
    response.mutable_status()->set_status(sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION);
}

//=============================================================================
// OscControlService
//=============================================================================

void OscControlService::GetSendIP(const sushi_rpc::GenericVoidValue& /*request*/,
                                  sushi_rpc::GenericStringValue& response)
{
    response.set_value(_controller->get_send_ip());
}

void OscControlService::GetSendPort(const sushi_rpc::GenericVoidValue& /*request*/,
                                    sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_send_port());
}

void OscControlService::GetReceivePort(const sushi_rpc::GenericVoidValue& /*request*/,
                                       sushi_rpc::GenericIntValue& response)
{
    response.set_value(_controller->get_receive_port());
}

void OscControlService::GetEnabledParameterOutputs(const sushi_rpc::GenericVoidValue& /*request*/,
                                                   sushi_rpc::OscParameterOutputList& response)
{
    auto enabled_outputs = _controller->get_enabled_parameter_outputs();
    for (const auto& path : enabled_outputs)
    {
        response.add_path(path);
    }
}

void OscControlService::EnableOutputForParameter(const sushi_rpc::ParameterIdentifier& request,
                                                 sushi_rpc::CommandResponse& response)
{
    auto status = _controller->enable_output_for_parameter(request.processor_id(), request.parameter_id());
    to_grpc(response, status);
}

void OscControlService::DisableOutputForParameter(const sushi_rpc::ParameterIdentifier& request,
                                                  sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disable_output_for_parameter(request.processor_id(), request.parameter_id());
    to_grpc(response, status);
}

void OscControlService::EnableAllOutput(const sushi_rpc::GenericVoidValue& /*request*/,
                                        sushi_rpc::CommandResponse& response)
{
    auto status = _controller->enable_all_output();
    to_grpc(response, status);
}

void OscControlService::DisableAllOutput(const sushi_rpc::GenericVoidValue& /*request*/,
                                         sushi_rpc::CommandResponse& response)
{
    auto status = _controller->disable_all_output();
    to_grpc(response, status);
}

//=============================================================================
// SessionControlService
//=============================================================================

void SessionControlService::SaveSession(const sushi_rpc::GenericVoidValue& /*request*/,
                                        sushi_rpc::SessionState& response)
{
    auto session_state = _controller->save_session();
    to_grpc(response, session_state);
}

void SessionControlService::RestoreSession(const sushi_rpc::SessionState& request,
                                           sushi_rpc::CommandResponse& response)
{
    sushi::control::SessionState sushi_state;
    to_sushi_ext(sushi_state, request);
    auto status = _controller->restore_session(sushi_state);
    to_grpc(response, status);
}


//=============================================================================
// NotificationControlService
//=============================================================================
constexpr int PUB_SOCKET_HWM = 20; // Number of messages kept before being dropped by the socket

NotificationControlService::NotificationControlService(sushi::control::SushiControl* controller) : _controller(controller)
{
}

NotificationControlService::~NotificationControlService()
{
    stop();
}

bool NotificationControlService::start(zmq::context_t& context, const std::string& socket)
{
    _socket = zmq::socket_t(context, ZMQ_PUB);
    _socket.set(zmq::sockopt::sndhwm, PUB_SOCKET_HWM);
    _socket.set(zmq::sockopt::linger, 0);

    _socket.bind(socket);
    return true;
}

void NotificationControlService::stop()
{
    if (_socket)
    {
        _socket.close();
    }
}

void NotificationControlService::notification(const sushi::control::ControlNotification* notification)
{
    switch(notification->type())
    {
        case sushi::control::NotificationType::TRANSPORT_UPDATE:
        {
            _forward_transport_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::CPU_TIMING_UPDATE:
        {
            _forward_cpu_timing_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::TRACK_UPDATE:
        {
            _forward_track_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::PROCESSOR_UPDATE:
        {
            _forward_processor_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::PARAMETER_CHANGE:
        {
            _forward_parameter_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::PROPERTY_CHANGE:
        {
            _forward_property_notification_to_subscribers(notification);
            break;
        }
        case sushi::control::NotificationType::ASYNC_COMMAND_COMPLETION:
        {
            _forward_async_command_notification_to_subscribers(notification);
            break;
        }
        default:
            break;
    }
}

void NotificationControlService::SubscribeToTransportChanges(const sushi_rpc::GenericVoidValue& /*request*/,
                                                             sushi_rpc::TransportUpdate& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::TRANSPORT_UPDATE, this);
    _transport_notifications_enabled = true;
}

void NotificationControlService::SubscribeToEngineCpuTimingUpdates(const sushi_rpc::GenericVoidValue& /*request*/,
                                                                   sushi_rpc::CpuTimings& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::CPU_TIMING_UPDATE, this);
    _cpu_timing_notifications_enabled = true;
}

void NotificationControlService::SubscribeToTrackChanges(const sushi_rpc::GenericVoidValue& /*request*/,
                                                         sushi_rpc::TrackUpdate& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::TRACK_UPDATE, this);
    _track_notifications_enabled = true;
}

void NotificationControlService::SubscribeToProcessorChanges(const sushi_rpc::GenericVoidValue& /*request*/,
                                                             sushi_rpc::ProcessorUpdate& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PROCESSOR_UPDATE, this);
    _processor_notifications_enabled = true;
}

void NotificationControlService::SubscribeToParameterUpdates(const sushi_rpc::ParameterNotificationBlocklist& /*request*/,
                                                             sushi_rpc::ParameterUpdate& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PARAMETER_CHANGE, this);
    _parameter_notifications_enabled = true;
}

void NotificationControlService::SubscribeToPropertyUpdates(const sushi_rpc::PropertyNotificationBlocklist& /*request*/,
                                                            sushi_rpc::PropertyValue& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PROPERTY_CHANGE, this);
    _property_notifications_enabled = true;
}

void NotificationControlService::SubscribeToAsyncCommandUpdates(const sushi_rpc::GenericVoidValue& /*request*/,
                                                                sushi_rpc::AsyncCommandResponse& /*response*/)
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::ASYNC_COMMAND_COMPLETION, this);
    _async_notifications_enabled = true;
}

void NotificationControlService::_forward_transport_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_transport_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::TransportNotification*>(notification);
        TransportUpdate message;
        auto action = typed_notification->action();

        switch(action)
        {
            case sushi::control::TransportAction::TEMPO_CHANGED:
            {
                float value = std::get<float>(typed_notification->value());
                message.set_tempo(value);
                break;
            }
            case sushi::control::TransportAction::PLAYING_MODE_CHANGED:
            {
                auto grpc_playing_mode = to_grpc(std::get<sushi::control::PlayingMode>(typed_notification->value()));
                message.mutable_playing_mode()->set_mode(grpc_playing_mode);
                break;
            }
            case sushi::control::TransportAction::SYNC_MODE_CHANGED:
            {
                auto grpc_sync_mode = to_grpc(std::get<sushi::control::SyncMode>(typed_notification->value()));
                message.mutable_sync_mode()->set_mode(grpc_sync_mode);
                break;
            }
            case sushi::control::TransportAction::TIME_SIGNATURE_CHANGED:
            {
                auto mutable_time_signature = message.mutable_time_signature();
                const auto source_time_signature = std::get<sushi::control::TimeSignature>(typed_notification->value());
                mutable_time_signature->set_denominator(source_time_signature.denominator);
                mutable_time_signature->set_numerator(source_time_signature.numerator);
                break;
            }
            default:
            {
                assert(false);
                return;;
            }
        }
        _send(sushi_ipc::Message::TRANSPORT_UPDATE, message);
    }
}

void NotificationControlService::_forward_cpu_timing_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_cpu_timing_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::CpuTimingNotification*>(notification);
        CpuTimings message;
        to_grpc(message, typed_notification->cpu_timings());
        _send(sushi_ipc::Message::CPU_TIMINGS, message);
    }
}

void NotificationControlService::_forward_track_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_track_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::TrackNotification*>(notification);
        TrackUpdate message;

        switch (typed_notification->action())
        {
            case sushi::control::TrackAction::ADDED:
                message.set_action(TrackUpdate_Action_TRACK_ADDED);
                break;
            case sushi::control::TrackAction::DELETED:
                message.set_action(TrackUpdate_Action_TRACK_DELETED);
                break;
            default:
                assert(false);
                return;
        }

        message.mutable_track()->set_id(typed_notification->track_id());
        _send(sushi_ipc::Message::TRACK_UPDATE, message);
    }
}

void NotificationControlService::_forward_processor_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_processor_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::ProcessorNotification*>(notification);
        ProcessorUpdate message;

        switch (typed_notification->action())
        {
            case sushi::control::ProcessorAction::ADDED:
                message.set_action(ProcessorUpdate_Action_PROCESSOR_ADDED);
                break;
            case sushi::control::ProcessorAction::DELETED:
                message.set_action(ProcessorUpdate_Action_PROCESSOR_DELETED);
                break;
            default:
                assert(false);
                return;
        }

        message.mutable_processor()->set_id(typed_notification->processor_id());
        message.mutable_parent_track()->set_id(typed_notification->parent_track_id());
        _send(sushi_ipc::Message::PROCESSOR_UPDATE, message);
    }
}

void NotificationControlService::_forward_parameter_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_parameter_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::ParameterChangeNotification*>(notification);
        ParameterUpdate message;
        message.set_normalized_value(typed_notification->value());
        message.set_domain_value(typed_notification->domain_value());
        message.set_formatted_value(typed_notification->formatted_value());
        message.mutable_parameter()->set_parameter_id(typed_notification->parameter_id());
        message.mutable_parameter()->set_processor_id(typed_notification->processor_id());
        _send(sushi_ipc::Message::PARAMETER_UPDATE, message);
    }
}

void NotificationControlService::_forward_property_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_property_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::PropertyChangeNotification*>(notification);
        PropertyValue message;
        message.set_value(typed_notification->value());
        message.mutable_property()->set_property_id(typed_notification->parameter_id());
        message.mutable_property()->set_processor_id(typed_notification->processor_id());
        _send(sushi_ipc::Message::PROPERTY_VALUE, message);
    }
}

void NotificationControlService::_forward_async_command_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    if (_async_notifications_enabled)
    {
        auto typed_notification = static_cast<const sushi::control::CommandCompletionNotification*>(notification);
        AsyncCommandResponse message;
        message.mutable_status()->set_status(to_grpc(typed_notification->status()));
        message.set_request_id(typed_notification->id());
        _send(sushi_ipc::Message::ASYNC_COMMAND_RESPONSE, message);
    }
}

void NotificationControlService::_send(sushi_ipc::Message command, grpc::protobuf::Message& request)
{
    // Reuse a static string to reduce the amount of allocations and heap fragmentation, notifications are called from the same thread in sushi
    static std::string serialised_message;

    int32_t command_code = static_cast<int32_t>(command);
    zmq::const_buffer command_buffer(&command_code, sizeof(command_code));

    request.SerializeToString(&serialised_message);
    zmq::const_buffer message_buffer(serialised_message.data(), serialised_message.size());
    try
    {
        _socket.send(command_buffer, zmq::send_flags::sndmore);
        _socket.send(message_buffer);
    }
    catch (zmq::error_t& e)
    {
        ELKLOG_LOG_ERROR("Error sending reply: {}", e.what());
    }
}

} // namespace sushi_ipc
