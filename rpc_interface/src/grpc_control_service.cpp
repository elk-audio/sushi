/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief Sushi Control Service, gRPC service for external control of Sushi
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "grpc_control_service.h"
#include "conversions.h"
#include "sushi/control_notifications.h"
#include "grpc_async_service_call_data.h"

namespace sushi_rpc {

inline grpc::Status to_grpc_status(sushi::control::ControlStatus status, const char* error = nullptr)
{
    if (!error)
    {
        error = to_string(status);
    }
    switch (status)
    {
        case sushi::control::ControlStatus::OK:
            return ::grpc::Status::OK;

        case sushi::control::ControlStatus::ERROR:
            return ::grpc::Status(::grpc::StatusCode::UNKNOWN, error);

        case sushi::control::ControlStatus::UNSUPPORTED_OPERATION:
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, error);

        case sushi::control::ControlStatus::NOT_FOUND:
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, error);

        case sushi::control::ControlStatus::OUT_OF_RANGE:
            return ::grpc::Status(::grpc::StatusCode::OUT_OF_RANGE, error);

        case sushi::control::ControlStatus::INVALID_ARGUMENTS:
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, error);

        default:
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, error);
    }
}

grpc::Status SystemControlService::GetSushiVersion(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::GenericStringValue* response)
{
    response->set_value(_controller->get_sushi_version());
    return grpc::Status::OK;
}

grpc::Status SystemControlService::GetSushiApiVersion(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::GenericVoidValue* /*request*/,
                                                      sushi_rpc::GenericStringValue* response)
{
    response->set_value(_controller->get_sushi_api_version());
    return grpc::Status::OK;
}

grpc::Status SystemControlService::GetBuildInfo(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                sushi_rpc::SushiBuildInfo* response)
{
    auto build_info = _controller->get_sushi_build_info();
    to_proto(*response, build_info);
    return grpc::Status::OK;
}

grpc::Status SystemControlService::GetInputAudioChannelCount(grpc::ServerContext* /*context*/,
                                                             const sushi_rpc::GenericVoidValue* /*request*/,
                                                             sushi_rpc::GenericIntValue* response)
{
    response->set_value(_controller->get_input_audio_channel_count());
    return grpc::Status::OK;
}

grpc::Status SystemControlService::GetOutputAudioChannelCount(grpc::ServerContext* /*context*/,
                                                              const sushi_rpc::GenericVoidValue* /*request*/,
                                                              sushi_rpc::GenericIntValue* response)
{
    response->set_value(_controller->get_output_audio_channel_count());
    return grpc::Status::OK;
}

grpc::Status TransportControlService::GetSamplerate(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::GenericVoidValue* /*request*/,
                                                    sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_samplerate());
    return grpc::Status::OK;
}

grpc::Status TransportControlService::GetPlayingMode(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::GenericVoidValue* /*request*/,
                                                     sushi_rpc::PlayingMode* response)
{
    response->set_mode(to_proto(_controller->get_playing_mode()));
    return grpc::Status::OK;
}

grpc::Status TransportControlService::GetSyncMode(grpc::ServerContext* /*context*/,
                                                  const sushi_rpc::GenericVoidValue* /*request*/,
                                                  sushi_rpc::SyncMode* response)
{
    response->set_mode(to_proto(_controller->get_sync_mode()));
    return grpc::Status::OK;
}

grpc::Status TransportControlService::GetTimeSignature(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::GenericVoidValue* /*request*/,
                                                       sushi_rpc::TimeSignature* response)
{
    auto ts = _controller->get_time_signature();
    response->set_denominator(ts.denominator);
    response->set_numerator(ts.numerator);
    return grpc::Status::OK;
}

grpc::Status TransportControlService::GetTempo(grpc::ServerContext* /*context*/,
                                               const sushi_rpc::GenericVoidValue* /*request*/,
                                               sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_tempo());
    return grpc::Status::OK;
}

grpc::Status TransportControlService::SetTempo(grpc::ServerContext* /*context*/,
                                               const sushi_rpc::GenericFloatValue* request,
                                               sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_tempo(request->value());
    to_proto(*response, status);
    return grpc::Status::OK;}

grpc::Status TransportControlService::SetPlayingMode(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::PlayingMode* request,
                                                     sushi_rpc::CommandResponse* response)
{
    _controller->set_playing_mode(to_sushi_ext(request->mode()));
    response->mutable_status()->set_status(CommandStatus::SUCCESS);
    return grpc::Status::OK;
}

grpc::Status TransportControlService::SetSyncMode(grpc::ServerContext* /*context*/,
                                                  const sushi_rpc::SyncMode*request,
                                                  sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_sync_mode(to_sushi_ext(request->mode()));
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status TransportControlService::SetTimeSignature(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::TimeSignature* request,
                                                       sushi_rpc::CommandResponse* response)
{
    sushi::control::TimeSignature ts;
    ts.numerator = request->numerator();
    ts.denominator = request->denominator();
    auto status = _controller->set_time_signature(ts);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status TimingControlService::GetTimingsEnabled(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::GenericVoidValue* /*request*/,
                                                     sushi_rpc::GenericBoolValue* response)
{
    response->set_value(_controller->get_timing_statistics_enabled());
    return grpc::Status::OK;
}

grpc::Status TimingControlService::SetTimingsEnabled(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::GenericBoolValue* request,
                                                     sushi_rpc::CommandResponse* response)
{
    _controller->set_timing_statistics_enabled(request->value());
    response->mutable_status()->set_status(CommandStatus::SUCCESS);
    return grpc::Status::OK;
}

grpc::Status TimingControlService::GetEngineTimings(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::GenericVoidValue* /*request*/,
                                                    sushi_rpc::CpuTimings* response)
{
    auto [status, timings] = _controller->get_engine_timings();
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response, timings);
    }
    return grpc::Status::OK;
}

grpc::Status TimingControlService::GetTrackTimings(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::TrackIdentifier* request,
                                                   sushi_rpc::TimingResponse* response)
{
    auto [status, timings] = _controller->get_track_timings(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_timings(), timings);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status TimingControlService::GetProcessorTimings(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::ProcessorIdentifier* request,
                                                       sushi_rpc::TimingResponse* response)
{
    auto [status, timings] = _controller->get_processor_timings(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_timings(), timings);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status TimingControlService::ResetAllTimings(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::CommandResponse* response)
{
    _controller->reset_all_timings();
    response->mutable_status()->set_status(CommandStatus::SUCCESS);
    return grpc::Status::OK;
}

grpc::Status TimingControlService::ResetTrackTimings(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::TrackIdentifier* request,
                                                     sushi_rpc::CommandResponse* response)
{
    auto status = _controller->reset_track_timings(request->id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status TimingControlService::ResetProcessorTimings(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ProcessorIdentifier* request,
                                                         sushi_rpc::CommandResponse* response)
{
    auto status = _controller->reset_processor_timings(request->id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendNoteOn(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::NoteOnRequest*request,
                                                sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_note_on(request->track().id(), request->channel(), request->note(), request->velocity());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendNoteOff(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::NoteOffRequest* request,
                                                 sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_note_off(request->track().id(), request->channel(), request->note(), request->velocity());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendNoteAftertouch(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::NoteAftertouchRequest* request,
                                                        sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_note_aftertouch(request->track().id(), request->channel(), request->note(), request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendAftertouch(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::NoteModulationRequest* request,
                                                    sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_aftertouch(request->track().id(), request->channel(), request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendPitchBend(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::NoteModulationRequest* request,
                                                   sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_pitch_bend(request->track().id(), request->channel(), request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status KeyboardControlService::SendModulation(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::NoteModulationRequest* request,
                                                    sushi_rpc::CommandResponse* response)
{
    auto status = _controller->send_modulation(request->track().id(), request->channel(), request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetAllProcessors(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::GenericVoidValue* /*request*/,
                                                        sushi_rpc::ProcessorInfoList* response)
{
    auto processors = _controller->get_all_processors();
    for (const auto& processor : processors)
    {
        auto info = response->add_processors();
        to_proto(*info, processor);
    }
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetAllTracks(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::GenericVoidValue* /*request*/,
                                                    sushi_rpc::TrackInfoList* response)
{
    auto tracks = _controller->get_all_tracks();
    for (const auto& track : tracks)
    {
        auto info = response->add_tracks();
        to_proto(*info, track);
    }
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetTrackId(grpc::ServerContext* /*context*/,
                                                  const sushi_rpc::GenericStringValue* request,
                                                  sushi_rpc::TrackIdentifierResponse* response)
{
    auto [status, id] = _controller->get_track_id(request->value());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_id(id);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetTrackInfo(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::TrackIdentifier* request,
                                                    sushi_rpc::TrackInfoResponse* response)
{
    auto [status, track] = _controller->get_track_info(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_info(), track);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetTrackProcessors(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::TrackIdentifier* request,
                                                          sushi_rpc::ProcessorInfoListResponse* response)
{
    auto [status, processors] = _controller->get_track_processors(request->id());
    for (const auto& processor : processors)
    {
        auto info = response->add_processors();
        to_proto(*info, processor);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetProcessorId(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::GenericStringValue* request,
                                                      sushi_rpc::ProcessorIdentifierResponse* response)
{
    auto [status, id] = _controller->get_processor_id(request->value());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_id(id);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetProcessorInfo(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::ProcessorIdentifier* request,
                                                        sushi_rpc::ProcessorInfoResponse* response)
{
    auto [status, processor] = _controller->get_processor_info(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_processor(), processor);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetProcessorBypassState(grpc::ServerContext* /*context*/,
                                                               const sushi_rpc::ProcessorIdentifier* request,
                                                               sushi_rpc::BoolResponse* response)
{
    auto [status, state] = _controller->get_processor_bypass_state(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(state);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::GetProcessorState(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ProcessorIdentifier* request,
                                                         sushi_rpc::ProcessorStateResponse* response)
{
    auto [status, state] = _controller->get_processor_state(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_state(), state);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::SetProcessorBypassState(grpc::ServerContext* /*context*/,
                                                               const sushi_rpc::ProcessorBypassStateSetRequest* request,
                                                               sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_processor_bypass_state(request->processor().id(), request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::SetProcessorState(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ProcessorStateSetRequest* request,
                                                         sushi_rpc::CommandResponse* response)
{
    sushi::control::ProcessorState sushi_state;
    to_sushi_ext(sushi_state, request->state());
    auto status = _controller->set_processor_state(request->processor().id(), sushi_state);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::CreateTrack(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::CreateTrackRequest* request,
                                                   sushi_rpc::CommandResponse* response)
{
    auto thread = (request->thread().has_value() ? std::optional<int>(request->thread().value()) : std::nullopt);
    auto status = _controller->create_track(request->name(), request->channels(), thread);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::CreateMultibusTrack(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::CreateMultibusTrackRequest* request,
                                                           sushi_rpc::CommandResponse* response)
{
    auto thread = (request->thread().has_value() ? std::optional<int>(request->thread().value()) : std::nullopt);
    auto status = _controller->create_multibus_track(request->name(), request->buses(), thread);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::CreatePreTrack(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::CreatePreTrackRequest* request,
                                                      sushi_rpc::CommandResponse* response)
{
    auto status = _controller->create_pre_track(request->name());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::CreatePostTrack(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::CreatePostTrackRequest* request,
                                                       sushi_rpc::CommandResponse* response)
{
    auto status = _controller->create_post_track(request->name());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::DeleteTrack(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::TrackIdentifier* request,
                                                   sushi_rpc::CommandResponse* response)
{
    auto status = _controller->delete_track(request->id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::CreateProcessorOnTrack(grpc::ServerContext* /*context*/,
                                                              const sushi_rpc::CreateProcessorRequest* request,
                                                              sushi_rpc::CommandResponse* response)
{
    std::optional<int> before_processor = std::nullopt;
    if (request->position().add_to_back() == false)
    {
        before_processor = request->position().before_processor().id();
    }
    auto status = _controller->create_processor_on_track(request->name(),
                                                         request->uid(),
                                                         request->path(),
                                                         to_sushi_ext(request->type().type()),
                                                         request->track().id(),
                                                         before_processor);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::MoveProcessorOnTrack(grpc::ServerContext* /*context*/,
                                                            const sushi_rpc::MoveProcessorRequest*request,
                                                            sushi_rpc::CommandResponse* response)
{
    std::optional<int> before_processor = std::nullopt;
    if (request->position().add_to_back() == false)
    {
        before_processor = request->position().before_processor().id();
    }
    auto status = _controller->move_processor_on_track(request->processor().id(),
                                                       request->source_track().id(),
                                                       request->dest_track().id(),
                                                       before_processor);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioGraphControlService::DeleteProcessorFromTrack(grpc::ServerContext* /*context*/,
                                                                const sushi_rpc::DeleteProcessorRequest* request,
                                                                sushi_rpc::CommandResponse* response)
{
    auto status = _controller->delete_processor_from_track(request->processor().id(),
                                                           request->track().id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetTrackParameters(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::TrackIdentifier* request,
                                                         sushi_rpc::ParameterInfoListResponse* response)
{
    auto [status, parameters] = _controller->get_track_parameters(request->id());
    for (const auto& parameter : parameters)
    {
        auto info = response->add_parameters();
        to_proto(*info, parameter);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetParameterId(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::ParameterIdRequest* request,
                                                     sushi_rpc::ParameterIdentifierResponse* response)
{
    auto [status, id] = _controller->get_parameter_id(request->processor().id(), request->parametername());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->mutable_id()->set_parameter_id(id);
        response->mutable_id()->set_processor_id(request->processor().id());
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetParameterInfo(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::ParameterIdentifier* request,
                                                       sushi_rpc::ParameterInfoResponse* response)
{
    auto [status, parameter] = _controller->get_parameter_info(request->processor_id(), request->parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_info(), parameter);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetParameterValue(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::ParameterIdentifier* request,
                                                        sushi_rpc::FloatResponse* response)
{
    auto [status, value] = _controller->get_parameter_value(request->processor_id(), request->parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(value);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetParameterValueInDomain(grpc::ServerContext* /*context*/,
                                                                const sushi_rpc::ParameterIdentifier* request,
                                                                sushi_rpc::FloatResponse* response)
{
    auto [status, value] = _controller->get_parameter_value_in_domain(request->processor_id(), request->parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(value);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetParameterValueAsString(grpc::ServerContext* /*context*/,
                                                            const sushi_rpc::ParameterIdentifier* request,
                                                            sushi_rpc::StringResponse* response)
{
    auto [status, value] = _controller->get_parameter_value_as_string(request->processor_id(), request->parameter_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(value);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::SetParameterValue(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::ParameterValue* request,
                                                        sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_parameter_value(request->parameter().processor_id(),
                                                   request->parameter().parameter_id(),
                                                   request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status ProgramControlService::GetProcessorCurrentProgram(grpc::ServerContext* /*context*/,
                                                               const sushi_rpc::ProcessorIdentifier* request,
                                                               sushi_rpc::ProgramIdentifierResponse* response)
{
    auto [status, program] = _controller->get_processor_current_program(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_program(program);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ProgramControlService::GetProcessorCurrentProgramName(grpc::ServerContext* /*context*/,
                                                                   const sushi_rpc::ProcessorIdentifier* request,
                                                                   sushi_rpc::StringResponse* response)
{
    auto [status, program] = _controller->get_processor_current_program_name(request->id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(program);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ProgramControlService::GetProcessorProgramName(grpc::ServerContext* /*context*/,
                                                            const sushi_rpc::ProcessorProgramIdentifier* request,
                                                            sushi_rpc::StringResponse* response)
{
    auto [status, program] = _controller->get_processor_program_name(request->processor().id(), request->program());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(program);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ProgramControlService::GetProcessorPrograms(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ProcessorIdentifier* request,
                                                         sushi_rpc::ProgramInfoListResponse* response)
{
    auto [status, programs] = _controller->get_processor_programs(request->id());
    int id = 0;
    for (auto& program : programs)
    {
        auto info = response->add_programs();
        info->set_name(program);
        info->mutable_id()->set_program(id++);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ProgramControlService::SetProcessorProgram(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::ProcessorProgramSetRequest* request,
                                                        sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_processor_program(request->processor().id(), request->program().program());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetProcessorParameters(grpc::ServerContext* /*context*/,
                                                             const sushi_rpc::ProcessorIdentifier* request,
                                                             sushi_rpc::ParameterInfoListResponse* response)
{
    auto [status, parameters] = _controller->get_processor_parameters(request->id());
    for (const auto& parameter : parameters)
    {
        auto info = response->add_parameters();
        to_proto(*info, parameter);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetTrackProperties(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::TrackIdentifier* request,
                                                         sushi_rpc::PropertyInfoListResponse* response)
{
    auto [status, properties] = _controller->get_track_properties(request->id());
    for (const auto& property : properties)
    {
        auto info = response->add_properties();
        to_proto(*info, property);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetProcessorProperties(grpc::ServerContext* /*context*/,
                                                             const sushi_rpc::ProcessorIdentifier* request,
                                                             sushi_rpc::PropertyInfoListResponse* response)
{
    auto [status, properties] = _controller->get_processor_properties(request->id());
    for (const auto& property : properties)
    {
        auto info = response->add_properties();
        to_proto(*info, property);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetPropertyId(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::PropertyIdRequest* request,
                                                    sushi_rpc::PropertyIdentifierResponse* response)
{
    auto [status, id] = _controller->get_property_id(request->processor().id(), request->property_name());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->mutable_id()->set_property_id(id);
        response->mutable_id()->set_processor_id(request->processor().id());
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetPropertyInfo(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::PropertyIdentifier* request,
                                                      sushi_rpc::PropertyInfoResponse* response)
{
    auto [status, property] = _controller->get_property_info(request->processor_id(), request->property_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        to_proto(*response->mutable_info(), property);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::GetPropertyValue(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::PropertyIdentifier* request,
                                                       sushi_rpc::StringResponse* response)
{
    auto [status, value] = _controller->get_property_value(request->processor_id(), request->property_id());
    if (status == sushi::control::ControlStatus::OK)
    {
        response->set_value(std::move(value));
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status ParameterControlService::SetPropertyValue(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::PropertyValue* request,
                                                       sushi_rpc::CommandResponse* response)
{
    auto status = _controller->set_property_value(request->property().processor_id(),
                                                  request->property().property_id(),
                                                  request->value());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetInputPorts(grpc::ServerContext* /*context*/,
                                               const sushi_rpc::GenericVoidValue* /*request*/,
                                               sushi_rpc::GenericIntValue* response)
{
    response->set_value(_midi_controller->get_input_ports());
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetOutputPorts(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                sushi_rpc::GenericIntValue* response)
{
    response->set_value(_midi_controller->get_output_ports());
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetAllKbdInputConnections(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::GenericVoidValue* /*request*/,
                                                           sushi_rpc::MidiKbdConnectionList* response)
{
    auto input_connections = _midi_controller->get_all_kbd_input_connections();
    for (const auto& connection : input_connections)
    {
        auto info = response->add_connections();
        to_proto(*info, connection);
    }
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetAllKbdOutputConnections(grpc::ServerContext* /*context*/,
                                                            const sushi_rpc::GenericVoidValue* /*request*/,
                                                            sushi_rpc::MidiKbdConnectionList* response)
{
    auto output_connections = _midi_controller->get_all_kbd_output_connections();
    for (const auto& connection : output_connections)
    {
        auto info = response->add_connections();
        to_proto(*info, connection);
    }
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetAllCCInputConnections(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::GenericVoidValue* /*request*/,
                                                          sushi_rpc::MidiCCConnectionList* response)
{
    auto output_connections = _midi_controller->get_all_cc_input_connections();
    for (const auto& connection : output_connections)
    {
        auto info = response->add_connections();
        to_proto(*info, connection);
    }
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetAllPCInputConnections(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::GenericVoidValue* /*request*/,
                                                          sushi_rpc::MidiPCConnectionList* response)
{
    auto input_connections = _midi_controller->get_all_pc_input_connections();
    for (const auto& connection : input_connections)
    {
        auto info = response->add_connections();
        to_proto(*info, connection);
    }
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetCCInputConnectionsForProcessor(grpc::ServerContext* /*context*/,
                                                                   const sushi_rpc::ProcessorIdentifier* request,
                                                                   sushi_rpc::MidiCCConnectionListResponse* response)
{
    const auto processor_id = request->id();
    auto output_connections = _midi_controller->get_cc_input_connections_for_processor(processor_id);
    if(output_connections.first == sushi::control::ControlStatus::OK)
    {
        for (const auto& connection : output_connections.second)
        {
            auto info = response->add_connections();
            to_proto(*info, connection);
        }
    }
    response->mutable_status()->set_status(to_proto(output_connections.first));
    return grpc::Status::OK;
}

grpc::Status MidiControlService::GetPCInputConnectionsForProcessor(grpc::ServerContext* /*context*/,
                                                                   const sushi_rpc::ProcessorIdentifier* request,
                                                                   sushi_rpc::MidiPCConnectionListResponse* response)
{
    const auto processor_id = request->id();
    auto input_connections = _midi_controller->get_pc_input_connections_for_processor(processor_id);
    if(input_connections.first == sushi::control::ControlStatus::OK)
    {
        for (const auto& connection : input_connections.second)
        {
            auto info = response->add_connections();
            to_proto(*info, connection);
        }
    }
    response->mutable_status()->set_status(to_proto(input_connections.first));
    return grpc::Status::OK;
}


grpc::Status MidiControlService::GetMidiClockOutputEnabled(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::GenericIntValue* request,
                                                           sushi_rpc::GenericBoolValue* response)
{
    response->set_value(_midi_controller->get_midi_clock_output_enabled(request->value()));
    return grpc::Status::OK;
}

grpc::Status MidiControlService::SetMidiClockOutputEnabled(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::MidiClockSetRequest* request,
                                                           sushi_rpc::CommandResponse* response)
{
    auto status = _midi_controller->set_midi_clock_output_enabled(request->enabled(), request->port());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::ConnectKbdInputToTrack(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::MidiKbdConnection* request,
                                                        sushi_rpc::CommandResponse* response)
{
    const auto track_id = request->track();
    const auto channel = request->channel().channel();
    const auto port = request->port();
    const auto raw_midi = request->raw_midi();
    const auto midi_channel = to_sushi_ext(channel);

    const auto status = _midi_controller->connect_kbd_input_to_track(track_id.id(), midi_channel, port, raw_midi);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::ConnectKbdOutputFromTrack(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::MidiKbdConnection* request,
                                                           sushi_rpc::CommandResponse* response)
{
    const auto track_id = request->track();
    const auto channel = request->channel().channel();
    const auto port = request->port();
    const auto midi_channel = to_sushi_ext(channel);

    const auto status = _midi_controller->connect_kbd_output_from_track(track_id.id(), midi_channel, port);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::ConnectCCToParameter(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::MidiCCConnection* request,
                                                      sushi_rpc::CommandResponse* response)
{
    const auto midi_channel = to_sushi_ext(request->channel().channel());
    const auto status = _midi_controller->connect_cc_to_parameter(request->parameter().processor_id(),
                                                                  request->parameter().parameter_id(),
                                                                  midi_channel,
                                                                  request->port(),
                                                                  request->cc_number(),
                                                                  request->min_range(),
                                                                  request->max_range(),
                                                                  request->relative_mode());

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::ConnectPCToProcessor(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::MidiPCConnection* request,
                                                      sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->processor().id();
    const MidiChannel_Channel channel = request->channel().channel();
    const auto port = request->port();

    sushi::control::MidiChannel midi_channel = to_sushi_ext(channel);

    const auto status = _midi_controller->connect_pc_to_processor(processor_id, midi_channel, port);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectKbdInput(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::MidiKbdConnection* request,
                                                    sushi_rpc::CommandResponse* response)
{
    const auto track_id = request->track();
    const auto channel = request->channel().channel();
    const auto port = request->port();
    const auto midi_channel = to_sushi_ext(channel);
    const auto raw_midi = request->raw_midi();
    const auto status = _midi_controller->disconnect_kbd_input(track_id.id(), midi_channel, port, raw_midi);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectKbdOutput(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::MidiKbdConnection* request,
                                                     sushi_rpc::CommandResponse* response)
{
    const auto track_id = request->track();
    const auto channel = request->channel().channel();
    const auto port = request->port();
    const auto midi_channel = to_sushi_ext(channel);

    const auto status = _midi_controller->disconnect_kbd_output(track_id.id(), midi_channel, port);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectCC(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::MidiCCConnection* request,
                                              sushi_rpc::CommandResponse* response)
{
    const auto midi_channel = to_sushi_ext(request->channel().channel());
    const auto status = _midi_controller->disconnect_cc(request->parameter().processor_id(),
                                                        midi_channel,
                                                        request->port(),
                                                        request->cc_number());

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectPC(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::MidiPCConnection* request,
                                              sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->processor().id();
    const MidiChannel_Channel channel = request->channel().channel();
    const auto port = request->port();
    sushi::control::MidiChannel midi_channel = to_sushi_ext(channel);

    const auto status = _midi_controller->disconnect_pc(processor_id, midi_channel, port);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectAllCCFromProcessor(grpc::ServerContext* /*context*/,
                                                              const sushi_rpc::ProcessorIdentifier* request,
                                                              sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->id();
    const auto status = _midi_controller->disconnect_all_cc_from_processor(processor_id);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status MidiControlService::DisconnectAllPCFromProcessor(grpc::ServerContext* /*context*/,
                                                              const sushi_rpc::ProcessorIdentifier* request,
                                                              sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->id();
    const auto status = _midi_controller->disconnect_all_pc_from_processor(processor_id);
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::GetAllInputConnections(grpc::ServerContext* /*context*/,
                                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                                sushi_rpc::AudioConnectionList* response)
{
    auto connections = _controller->get_all_input_connections();
    for (const auto& connection : connections)
    {
        auto c = response->add_connections();
        to_proto(*c, connection);
    }
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::GetAllOutputConnections(grpc::ServerContext* /*context*/,
                                                                 const sushi_rpc::GenericVoidValue* /*request*/,
                                                                 sushi_rpc::AudioConnectionList* response)
{
    auto connections = _controller->get_all_output_connections();
    for (const auto& connection : connections)
    {
        auto c = response->add_connections();
        to_proto(*c, connection);
    }
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::GetInputConnectionsForTrack(grpc::ServerContext* /*context*/,
                                                                     const sushi_rpc::TrackIdentifier* request,
                                                                     sushi_rpc::AudioConnectionListResponse* response)
{
    auto [status, connections] = _controller->get_input_connections_for_track(request->id());
    for (const auto& connection : connections)
    {
        auto c = response->add_connections();
        to_proto(*c, connection);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::GetOutputConnectionsForTrack(grpc::ServerContext* /*context*/,
                                                                      const sushi_rpc::TrackIdentifier* request,
                                                                      sushi_rpc::AudioConnectionListResponse* response)
{
    auto [status, connections] = _controller->get_output_connections_for_track(request->id());
    for (const auto& connection : connections)
    {
        auto c = response->add_connections();
        to_proto(*c, connection);
    }
    response->mutable_status()->set_status(to_proto(status));
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::ConnectInputChannelToTrack(grpc::ServerContext* /*context*/,
                                                                    const sushi_rpc::AudioConnection* request,
                                                                    sushi_rpc::CommandResponse* response)
{
    auto status = _controller->connect_input_channel_to_track(request->track().id(),
                                                              request->track_channel(),
                                                              request->engine_channel());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::ConnectOutputChannelFromTrack(grpc::ServerContext* /*context*/,
                                                                       const sushi_rpc::AudioConnection* request,
                                                                       sushi_rpc::CommandResponse* response)
{
    auto status = _controller->connect_output_channel_to_track(request->track().id(),
                                                               request->track_channel(),
                                                               request->engine_channel());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::DisconnectInput(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::AudioConnection* request,
                                                         sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disconnect_input(request->track().id(),
                                                request->track_channel(),
                                                request->engine_channel());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::DisconnectOutput(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::AudioConnection* request,
                                                          sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disconnect_output(request->track().id(),
                                                 request->track_channel(),
                                                 request->engine_channel());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status AudioRoutingControlService::DisconnectAllInputsFromTrack(grpc::ServerContext* /*context*/,
                                                                      const sushi_rpc::TrackIdentifier* request,
                                                                      sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disconnect_all_inputs_from_track(request->id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

// This function is deprecated and should be removed eventually.
/*grpc::Status AudioRoutingControlService::DisconnectAllOutputFromTrack(grpc::ServerContext* context,
                                                                      const sushi_rpc::TrackIdentifier* request,
                                                                      sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disconnect_all_outputs_from_track(request->id());
    return to_grpc_status(status);
}*/

grpc::Status AudioRoutingControlService::DisconnectAllOutputsFromTrack(grpc::ServerContext* /*context*/,
                                                                       const sushi_rpc::TrackIdentifier* request,
                                                                       sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disconnect_all_outputs_from_track(request->id());
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status CvGateControlService::GetCvInputChannelCount(grpc::ServerContext* context,
                                                          const sushi_rpc::GenericVoidValue* request,
                                                          sushi_rpc::GenericIntValue* response)
{
    return Service::GetCvInputChannelCount(context, request, response);
}

grpc::Status CvGateControlService::GetCvOutputChannelCount(grpc::ServerContext* context,
                                                           const sushi_rpc::GenericVoidValue* request,
                                                           sushi_rpc::GenericIntValue* response)
{
    return Service::GetCvOutputChannelCount(context, request, response);
}

grpc::Status CvGateControlService::GetAllCvInputConnections(grpc::ServerContext* context,
                                                            const sushi_rpc::GenericVoidValue* request,
                                                            sushi_rpc::CvConnectionList* response)
{
    return Service::GetAllCvInputConnections(context, request, response);
}

grpc::Status CvGateControlService::GetAllCvOutputConnections(grpc::ServerContext* context,
                                                             const sushi_rpc::GenericVoidValue* request,
                                                             sushi_rpc::CvConnectionList* response)
{
    return Service::GetAllCvOutputConnections(context, request, response);
}

grpc::Status CvGateControlService::GetAllGateInputConnections(grpc::ServerContext* context,
                                                              const sushi_rpc::GenericVoidValue* request,
                                                              sushi_rpc::GateConnectionList* response)
{
    return Service::GetAllGateInputConnections(context, request, response);
}

grpc::Status CvGateControlService::GetAllGateOutputConnections(grpc::ServerContext* context,
                                                               const sushi_rpc::GenericVoidValue* request,
                                                               sushi_rpc::GateConnectionList* response)
{
    return Service::GetAllGateOutputConnections(context, request, response);
}

grpc::Status CvGateControlService::GetCvInputConnectionsForProcessor(grpc::ServerContext* context,
                                                                     const sushi_rpc::ProcessorIdentifier* request,
                                                                     sushi_rpc::CvConnectionListResponse* response)
{
    return Service::GetCvInputConnectionsForProcessor(context, request, response);
}

grpc::Status CvGateControlService::GetCvOutputConnectionsForProcessor(grpc::ServerContext* context,
                                                                      const sushi_rpc::ProcessorIdentifier* request,
                                                                      sushi_rpc::CvConnectionListResponse* response)
{
    return Service::GetCvOutputConnectionsForProcessor(context, request, response);
}

grpc::Status CvGateControlService::GetGateInputConnectionsForProcessor(grpc::ServerContext* context,
                                                                       const sushi_rpc::ProcessorIdentifier* request,
                                                                       sushi_rpc::GateConnectionListResponse* response)
{
    return Service::GetGateInputConnectionsForProcessor(context, request, response);
}

grpc::Status CvGateControlService::GetGateOutputConnectionsForProcessor(grpc::ServerContext* context,
                                                                        const sushi_rpc::ProcessorIdentifier* request,
                                                                        sushi_rpc::GateConnectionListResponse* response)
{
    return Service::GetGateOutputConnectionsForProcessor(context, request, response);
}

grpc::Status CvGateControlService::ConnectCvInputToParameter(grpc::ServerContext* context,
                                                             const sushi_rpc::CvConnection* request,
                                                             sushi_rpc::CommandResponse* response)
{
    return Service::ConnectCvInputToParameter(context, request, response);
}

grpc::Status CvGateControlService::ConnectCvOutputFromParameter(grpc::ServerContext* context,
                                                                const sushi_rpc::CvConnection* request,
                                                                sushi_rpc::CommandResponse* response)
{
    return Service::ConnectCvOutputFromParameter(context, request, response);
}

grpc::Status CvGateControlService::ConnectGateInputToProcessor(grpc::ServerContext* context,
                                                               const sushi_rpc::GateConnection* request,
                                                               sushi_rpc::CommandResponse* response)
{
    return Service::ConnectGateInputToProcessor(context, request, response);
}

grpc::Status CvGateControlService::ConnectGateOutputFromProcessor(grpc::ServerContext* context,
                                                                  const sushi_rpc::GateConnection* request,
                                                                  sushi_rpc::CommandResponse* response)
{
    return Service::ConnectGateOutputFromProcessor(context, request, response);
}

grpc::Status CvGateControlService::DisconnectCvInput(grpc::ServerContext* context,
                                                     const sushi_rpc::CvConnection* request,
                                                     sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectCvInput(context, request, response);
}

grpc::Status CvGateControlService::DisconnectCvOutput(grpc::ServerContext* context,
                                                      const sushi_rpc::CvConnection* request,
                                                      sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectCvOutput(context, request, response);
}

grpc::Status CvGateControlService::DisconnectGateInput(grpc::ServerContext* context,
                                                       const sushi_rpc::GateConnection* request,
                                                       sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectGateInput(context, request, response);
}

grpc::Status CvGateControlService::DisconnectGateOutput(grpc::ServerContext* context,
                                                        const sushi_rpc::GateConnection* request,
                                                        sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectGateOutput(context, request, response);
}

grpc::Status CvGateControlService::DisconnectAllCvInputsFromProcessor(grpc::ServerContext* context,
                                                                      const sushi_rpc::ProcessorIdentifier* request,
                                                                      sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectAllCvInputsFromProcessor(context, request, response);
}

grpc::Status CvGateControlService::DisconnectAllCvOutputsFromProcessor(grpc::ServerContext* context,
                                                                       const sushi_rpc::ProcessorIdentifier* request,
                                                                       sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectAllCvOutputsFromProcessor(context, request, response);
}

grpc::Status CvGateControlService::DisconnectAllGateInputsFromProcessor(grpc::ServerContext* context,
                                                                        const sushi_rpc::ProcessorIdentifier* request,
                                                                        sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectAllGateInputsFromProcessor(context, request, response);
}

grpc::Status CvGateControlService::DisconnectAllGateOutputsFromProcessor(grpc::ServerContext* context,
                                                                         const sushi_rpc::ProcessorIdentifier* request,
                                                                         sushi_rpc::CommandResponse* response)
{
    return Service::DisconnectAllGateOutputsFromProcessor(context, request, response);
}

grpc::Status OscControlService::GetSendIP(grpc::ServerContext* /*context*/,
                                          const sushi_rpc::GenericVoidValue* /*request*/,
                                          sushi_rpc::GenericStringValue* response)
{
  response->set_value(_controller->get_send_ip());
  return grpc::Status::OK;
}

grpc::Status OscControlService::GetSendPort(grpc::ServerContext* /*context*/,
                                            const sushi_rpc::GenericVoidValue* /*request*/,
                                            sushi_rpc::GenericIntValue* response)
{
    response->set_value(_controller->get_send_port());
    return grpc::Status::OK;
}

grpc::Status OscControlService::GetReceivePort(grpc::ServerContext* /*context*/,
                                               const sushi_rpc::GenericVoidValue* /*request*/,
                                               sushi_rpc::GenericIntValue* response)
{
    response->set_value(_controller->get_receive_port());
    return grpc::Status::OK;
}

grpc::Status OscControlService::GetEnabledParameterOutputs(grpc::ServerContext* /*context*/,
                                                           const sushi_rpc::GenericVoidValue* /*request*/,
                                                           sushi_rpc::OscParameterOutputList* response)
{
    auto enabled_outputs = _controller->get_enabled_parameter_outputs();

    for (const auto& path : enabled_outputs)
    {
        response->add_path(path);
    }
    return grpc::Status::OK;
}

grpc::Status OscControlService::EnableOutputForParameter(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ParameterIdentifier* request,
                                                         sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->processor_id();
    const auto parameter_id = request->parameter_id();

    auto status = _controller->enable_output_for_parameter(processor_id, parameter_id);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status OscControlService::DisableOutputForParameter(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::ParameterIdentifier* request,
                                                          sushi_rpc::CommandResponse* response)
{
    const auto processor_id = request->processor_id();
    const auto parameter_id = request->parameter_id();

    auto status = _controller->disable_output_for_parameter(processor_id, parameter_id);

    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status OscControlService::EnableAllOutput(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                sushi_rpc::CommandResponse* response)
{
    auto status = _controller->enable_all_output();
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status OscControlService::DisableAllOutput(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::GenericVoidValue* /*request*/,
                                                 sushi_rpc::CommandResponse* response)
{
    auto status = _controller->disable_all_output();
    to_proto(*response, status);
    return grpc::Status::OK;
}

grpc::Status SessionControlService::SaveSession(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                sushi_rpc::SessionState* response)
{
    auto session_state = _controller->save_session();
    to_proto(*response, session_state);
    return grpc::Status::OK;
}

grpc::Status SessionControlService::RestoreSession(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::SessionState* request,
                                                   sushi_rpc::CommandResponse* response)
{
    sushi::control::SessionState sushi_state;
    to_sushi_ext(sushi_state, *request);

    auto status = _controller->restore_session(sushi_state);

    to_proto(*response, status);
    return grpc::Status::OK;
}

NotificationControlService::NotificationControlService(sushi::control::SushiControl* controller) : _controller{controller},
                                                                                               _audio_graph_controller{controller->audio_graph_controller()}
{
    _controller->subscribe_to_notifications(sushi::control::NotificationType::TRANSPORT_UPDATE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::CPU_TIMING_UPDATE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::TRACK_UPDATE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PROCESSOR_UPDATE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PARAMETER_CHANGE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::PROPERTY_CHANGE, this);
    _controller->subscribe_to_notifications(sushi::control::NotificationType::ASYNC_COMMAND_COMPLETION, this);
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

void NotificationControlService::_forward_transport_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::TransportNotification*>(notification);
    auto notification_content = std::make_shared<TransportUpdate>();
    auto action = typed_notification->action();

    switch(action)
    {
        case sushi::control::TransportAction::TEMPO_CHANGED:
        {
            float value = std::get<float>(typed_notification->value());
            notification_content->set_tempo(value);
            break;
        }
        case sushi::control::TransportAction::PLAYING_MODE_CHANGED:
        {
            auto grpc_playing_mode = to_proto(std::get<sushi::control::PlayingMode>(typed_notification->value()));
            notification_content->mutable_playing_mode()->set_mode(grpc_playing_mode);
            break;
        }
        case sushi::control::TransportAction::SYNC_MODE_CHANGED:
        {
            auto grpc_sync_mode = to_proto(std::get<sushi::control::SyncMode>(typed_notification->value()));
            notification_content->mutable_sync_mode()->set_mode(grpc_sync_mode);
            break;
        }
        case sushi::control::TransportAction::TIME_SIGNATURE_CHANGED:
        {
            auto mutable_time_signature = notification_content->mutable_time_signature();
            const auto source_time_signature = std::get<sushi::control::TimeSignature>(typed_notification->value());
            mutable_time_signature->set_denominator(source_time_signature.denominator);
            mutable_time_signature->set_numerator(source_time_signature.numerator);
            break;
        }
        default:
        {
            assert(false);
            break;
        }
    }

    std::scoped_lock lock(_transport_subscriber_lock);
    for (auto& subscriber : _transport_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_cpu_timing_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::CpuTimingNotification*>(notification);
    auto notification_content = std::make_shared<CpuTimings>();
    to_proto(*notification_content, typed_notification->cpu_timings());

    std::scoped_lock lock(_timing_subscriber_lock);
    for (auto& subscriber : _timing_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_track_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::TrackNotification*>(notification);
    auto notification_content = std::make_shared<TrackUpdate>();
    auto action = typed_notification->action();

    switch(action)
    {
        case sushi::control::TrackAction::ADDED:
        {
            notification_content->set_action(TrackUpdate_Action_TRACK_ADDED);
            break;
        }
        case sushi::control::TrackAction::DELETED:
        {
            notification_content->set_action(TrackUpdate_Action_TRACK_DELETED);
            break;
        }
        default:
        {
            assert(false);
            break;
        }
    }

    notification_content->mutable_track()->set_id(typed_notification->track_id());

    std::scoped_lock lock(_track_subscriber_lock);
    for (auto& subscriber : _track_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_processor_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::ProcessorNotification*>(notification);
    auto notification_content = std::make_shared<ProcessorUpdate>();
    auto action = typed_notification->action();

    switch(action)
    {
        case sushi::control::ProcessorAction::ADDED:
        {
            notification_content->set_action(ProcessorUpdate_Action_PROCESSOR_ADDED);
            break;
        }
        case sushi::control::ProcessorAction::DELETED:
        {
            notification_content->set_action(ProcessorUpdate_Action_PROCESSOR_DELETED);
            break;
        }
        default:
        {
            assert(false);
            break;
        }
    }

    notification_content->mutable_processor()->set_id(typed_notification->processor_id());
    notification_content->mutable_parent_track()->set_id(typed_notification->parent_track_id());

    std::scoped_lock lock(_processor_subscriber_lock);
    for (auto& subscriber : _processor_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_parameter_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::ParameterChangeNotification*>(notification);
    auto notification_content = std::make_shared<ParameterUpdate>();
    notification_content->set_normalized_value(typed_notification->value());
    notification_content->set_domain_value(typed_notification->domain_value());
    notification_content->set_formatted_value(typed_notification->formatted_value());
    notification_content->mutable_parameter()->set_parameter_id(typed_notification->parameter_id());
    notification_content->mutable_parameter()->set_processor_id(typed_notification->processor_id());

    std::scoped_lock lock(_parameter_subscriber_lock);
    for (auto& subscriber : _parameter_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_property_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::PropertyChangeNotification*>(notification);
    auto notification_content = std::make_shared<PropertyValue>();
    notification_content->set_value(typed_notification->value());
    notification_content->mutable_property()->set_property_id(typed_notification->parameter_id());
    notification_content->mutable_property()->set_processor_id(typed_notification->processor_id());

    std::scoped_lock lock(_property_subscriber_lock);
    for (auto& subscriber : _property_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::_forward_async_command_notification_to_subscribers(const sushi::control::ControlNotification* notification)
{
    auto typed_notification = static_cast<const sushi::control::CommandCompletionNotification*>(notification);
    auto notification_content = std::make_shared<AsyncCommandResponse>();
    notification_content->mutable_status()->set_status(to_proto(typed_notification->status()));
    notification_content->set_request_id(typed_notification->id());

    std::scoped_lock lock(_command_subscriber_lock);
    for (auto& subscriber : _command_subscribers)
    {
        subscriber->push(notification_content);
    }
}

void NotificationControlService::subscribe(SubscribeToTransportChangesCallData* subscriber)
{
    std::scoped_lock lock(_transport_subscriber_lock);
    _transport_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToTransportChangesCallData* subscriber)
{
    std::scoped_lock lock(_transport_subscriber_lock);
    _transport_subscribers.erase(std::remove(_transport_subscribers.begin(),
                                             _transport_subscribers.end(),
                                             subscriber));
}

void NotificationControlService::subscribe(SubscribeToCpuTimingUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_timing_subscriber_lock);
    _timing_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToCpuTimingUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_timing_subscriber_lock);
    _timing_subscribers.erase(std::remove(_timing_subscribers.begin(),
                                          _timing_subscribers.end(),
                                          subscriber));
}

void NotificationControlService::subscribe(SubscribeToTrackChangesCallData* subscriber)
{
    std::scoped_lock lock(_track_subscriber_lock);
    _track_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToTrackChangesCallData* subscriber)
{
    std::scoped_lock lock(_track_subscriber_lock);
    _track_subscribers.erase(std::remove(_track_subscribers.begin(),
                                         _track_subscribers.end(),
                                         subscriber));
}

void NotificationControlService::subscribe(SubscribeToProcessorChangesCallData* subscriber)
{
    std::scoped_lock lock(_processor_subscriber_lock);
    _processor_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToProcessorChangesCallData* subscriber)
{
    std::scoped_lock lock(_processor_subscriber_lock);
    _processor_subscribers.erase(std::remove(_processor_subscribers.begin(),
                                             _processor_subscribers.end(),
                                             subscriber));
}

void NotificationControlService::subscribe(SubscribeToParameterUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_parameter_subscriber_lock);
    _parameter_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToParameterUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_parameter_subscriber_lock);
    _parameter_subscribers.erase(std::remove(_parameter_subscribers.begin(),
                                             _parameter_subscribers.end(),
                                             subscriber));
}

void NotificationControlService::subscribe(SubscribeToPropertyUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_property_subscriber_lock);
    _property_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToPropertyUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_property_subscriber_lock);
    _property_subscribers.erase(std::remove(_property_subscribers.begin(),
                                            _property_subscribers.end(),
                                            subscriber));
}

void NotificationControlService::subscribe(SubscribeToAsyncCommandUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_command_subscriber_lock);
    _command_subscribers.push_back(subscriber);
}

void NotificationControlService::unsubscribe(SubscribeToAsyncCommandUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_command_subscriber_lock);
    _command_subscribers.erase(std::remove(_command_subscribers.begin(),
                                           _command_subscribers.end(),
                                           subscriber));
}

void NotificationControlService::delete_all_subscribers()
{
    /* Unsubscribe and delete CallData subscribers directly, without
     * waiting for them to be asynchronously deleted when a worker
     * thread pulls them of the completion queue. */
    {
        std::scoped_lock lock(_transport_subscriber_lock);
        for (auto& subscriber : _transport_subscribers)
        {
            delete subscriber;
        }
        _transport_subscribers.clear();
    }

    {
        std::scoped_lock lock(_timing_subscriber_lock);
        for (auto& subscriber : _timing_subscribers)
        {
            delete subscriber;
        }
        _timing_subscribers.clear();
    }

    {
        std::scoped_lock lock(_track_subscriber_lock);
        for (auto& subscriber : _track_subscribers)
        {
            delete subscriber;
        }
        _track_subscribers.clear();
    }

    {
        std::scoped_lock lock(_parameter_subscriber_lock);
        for (auto& subscriber : _parameter_subscribers)
        {
            delete subscriber;
        }
        _parameter_subscribers.clear();
    }

    {
        std::scoped_lock lock(_property_subscriber_lock);
        for (auto& subscriber : _property_subscribers)
        {
            delete subscriber;
        }
        _processor_subscribers.clear();
    }

    {
        std::scoped_lock lock(_processor_subscriber_lock);
        for (auto& subscriber : _processor_subscribers)
        {
            delete subscriber;
        }
        _processor_subscribers.clear();
    }

    {
        std::scoped_lock lock(_command_subscriber_lock);
        for (auto& subscriber : _command_subscribers)
        {
            delete subscriber;
        }
        _command_subscribers.clear();
    }
}


} // sushi_rpc
