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
 * @brief Sushi Control Service, gRPC service for external control of Sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */


#include "../../include/control_notifications.h"
#include "control_service.h"
#include "async_service_call_data.h"

namespace sushi_rpc {

/* Convenience conversion functions between sushi enums and their respective grpc implementations */
inline sushi_rpc::ParameterType::Type to_grpc(const sushi::ext::ParameterType type)
{
    switch (type)
    {
        case sushi::ext::ParameterType::FLOAT:            return sushi_rpc::ParameterType::FLOAT;
        case sushi::ext::ParameterType::INT:              return sushi_rpc::ParameterType::INT;
        case sushi::ext::ParameterType::BOOL:             return sushi_rpc::ParameterType::BOOL;
        case sushi::ext::ParameterType::STRING_PROPERTY:  return sushi_rpc::ParameterType::STRING_PROPERTY;;
        case sushi::ext::ParameterType::DATA_PROPERTY:    return sushi_rpc::ParameterType::DATA_PROPERTY;;
        default:                                          return sushi_rpc::ParameterType::FLOAT;
    }
}

inline sushi_rpc::PlayingMode::Mode to_grpc(const sushi::ext::PlayingMode mode)
{
    switch (mode)
    {
        case sushi::ext::PlayingMode::STOPPED:      return sushi_rpc::PlayingMode::STOPPED;
        case sushi::ext::PlayingMode::PLAYING:      return sushi_rpc::PlayingMode::PLAYING;
        case sushi::ext::PlayingMode::RECORDING:    return sushi_rpc::PlayingMode::RECORDING;
        default:                                    return sushi_rpc::PlayingMode::PLAYING;
    }
}

inline sushi::ext::PlayingMode to_sushi_ext(const sushi_rpc::PlayingMode::Mode mode)
{
    switch (mode)
    {
        case sushi_rpc::PlayingMode::STOPPED:   return sushi::ext::PlayingMode::STOPPED;
        case sushi_rpc::PlayingMode::PLAYING:   return sushi::ext::PlayingMode::PLAYING;
        case sushi_rpc::PlayingMode::RECORDING: return sushi::ext::PlayingMode::RECORDING;
        default:                                return sushi::ext::PlayingMode::PLAYING;
    }
}

inline sushi_rpc::SyncMode::Mode to_grpc(const sushi::ext::SyncMode mode)
{
    switch (mode)
    {
        case sushi::ext::SyncMode::INTERNAL: return sushi_rpc::SyncMode::INTERNAL;
        case sushi::ext::SyncMode::MIDI:     return sushi_rpc::SyncMode::MIDI;
        case sushi::ext::SyncMode::LINK:     return sushi_rpc::SyncMode::LINK;
        default:                             return sushi_rpc::SyncMode::INTERNAL;
    }
}

inline sushi::ext::SyncMode to_sushi_ext(const sushi_rpc::SyncMode::Mode mode)
{
    switch (mode)
    {
        case sushi_rpc::SyncMode::INTERNAL: return sushi::ext::SyncMode::INTERNAL;
        case sushi_rpc::SyncMode::MIDI:     return sushi::ext::SyncMode::MIDI;
        case sushi_rpc::SyncMode::LINK:     return sushi::ext::SyncMode::LINK;
        default:                            return sushi::ext::SyncMode::INTERNAL;
    }
}

inline const char* to_string(const sushi::ext::ControlStatus status)
{
   switch (status)
    {
        case sushi::ext::ControlStatus::OK:                    return "OK";
        case sushi::ext::ControlStatus::ERROR:                 return "ERROR";
        case sushi::ext::ControlStatus::UNSUPPORTED_OPERATION: return "UNSUPPORTED OPERATION";
        case sushi::ext::ControlStatus::NOT_FOUND:             return "NOT FOUND";
        case sushi::ext::ControlStatus::OUT_OF_RANGE:          return "OUT OF RANGE";
        case sushi::ext::ControlStatus::INVALID_ARGUMENTS:     return "INVALID ARGUMENTS";
        default:                                               return "INTERNAL";
    }
}

inline grpc::Status to_grpc_status(sushi::ext::ControlStatus status, const char* error = nullptr)
{
    if (!error)
    {
        error = to_string(status);
    }
    switch (status)
    {
        case sushi::ext::ControlStatus::OK:
            return ::grpc::Status::OK;

        case sushi::ext::ControlStatus::ERROR:
            return ::grpc::Status(::grpc::StatusCode::UNKNOWN, error);

        case sushi::ext::ControlStatus::UNSUPPORTED_OPERATION:
            return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION, error);

        case sushi::ext::ControlStatus::NOT_FOUND:
            return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, error);

        case sushi::ext::ControlStatus::OUT_OF_RANGE:
            return ::grpc::Status(::grpc::StatusCode::OUT_OF_RANGE, error);

        case sushi::ext::ControlStatus::INVALID_ARGUMENTS:
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, error);

        default:
            return ::grpc::Status(::grpc::StatusCode::INTERNAL, error);
    }
}

inline void to_grpc(ParameterInfo& dest, const sushi::ext::ParameterInfo& src)
{
    dest.set_id(src.id);
    dest.mutable_type()->set_type(to_grpc(src.type));
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_unit(src.unit);
    dest.set_automatable(src.automatable);
    dest.set_min_range(src.min_range);
    dest.set_max_range(src.max_range);
}

inline void to_grpc(sushi_rpc::ProcessorInfo& dest, const sushi::ext::ProcessorInfo& src)
{
    dest.set_id(src.id);
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_parameter_count(src.parameter_count);
    dest.set_program_count(src.program_count);
}

inline void to_grpc(sushi_rpc::TrackInfo& dest, const sushi::ext::TrackInfo& src)
{
    dest.set_id(src.id);
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_input_channels(src.input_channels);
    dest.set_input_busses(src.input_busses);
    dest.set_output_channels(src.output_channels);
    dest.set_output_busses(src.output_busses);
    dest.set_processor_count(src.processor_count);
}

inline void to_grpc(sushi_rpc::CpuTimings& dest, const sushi::ext::CpuTimings& src)
{
    dest.set_average(src.avg);
    dest.set_min(src.min);
    dest.set_max(src.max);
}

grpc::Status SushiControlService::GetSamplerate(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::GenericVoidValue* /*request*/,
                                                sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_samplerate());
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetPlayingMode(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::GenericVoidValue* /*request*/,
                                                 sushi_rpc::PlayingMode* response)
{
    response->set_mode(to_grpc(_controller->get_playing_mode()));
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetPlayingMode(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::PlayingMode* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    _controller->set_playing_mode(to_sushi_ext(request->mode()));
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetSyncMode(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::GenericVoidValue* /*request*/,
                                              sushi_rpc::SyncMode* response)
{
    response->set_mode(to_grpc(_controller->get_sync_mode()));
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetSyncMode(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::SyncMode*request,
                                              sushi_rpc::GenericVoidValue* /*response*/)
{
    _controller->set_sync_mode(to_sushi_ext(request->mode()));
    // TODO - set_sync_mode should return a status, not void
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetTempo(grpc::ServerContext* /*context*/,
                                           const sushi_rpc::GenericVoidValue* /*request*/,
                                           sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_tempo());
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetTempo(grpc::ServerContext* /*context*/,
                                           const sushi_rpc::GenericFloatValue* request,
                                           sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_tempo(request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetTimeSignature(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::TimeSignature* response)
{
    auto ts = _controller->get_time_signature();
    response->set_denominator(ts.denominator);
    response->set_numerator(ts.numerator);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetTimeSignature(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::TimeSignature* request,
                                                   sushi_rpc::GenericVoidValue* /*response*/)
{
    sushi::ext::TimeSignature ts;
    ts.numerator = request->numerator();
    ts.denominator = request->denominator();
    auto status = _controller->set_time_signature(ts);
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetTracks(grpc::ServerContext* /*context*/,
                                            const sushi_rpc::GenericVoidValue* /*request*/,
                                            sushi_rpc::TrackInfoList* response)
{
    auto tracks = _controller->get_tracks();
    for (const auto& track : tracks)
    {
        auto info = response->add_tracks();
        to_grpc(*info, track);
    }
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SendNoteOn(grpc::ServerContext* /*context*/,
                                             const sushi_rpc::NoteOnRequest*request,
                                             sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_on(request->track().id(), request->channel(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteOff(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::NoteOffRequest* request,
                                              sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_off(request->track().id(), request->channel(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteAftertouch(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::NoteAftertouchRequest* request,
                                                     sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_aftertouch(request->track().id(), request->channel(), request->note(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendAftertouch(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_aftertouch(request->track().id(), request->channel(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendPitchBend(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::NoteModulationRequest* request,
                                                sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_pitch_bend(request->track().id(), request->channel(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendModulation(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_modulation(request->track().id(), request->channel(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetEngineTimings(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::CpuTimings* response)
{
    auto [status, timings] = _controller->get_engine_timings();
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    to_grpc(*response, timings);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetTrackTimings(grpc::ServerContext* /*context*/,
                                                  const sushi_rpc::TrackIdentifier* request,
                                                  sushi_rpc::CpuTimings* response)
{
    auto [status, timings] = _controller->get_track_timings(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    to_grpc(*response, timings);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorTimings(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::ProcessorIdentifier* request,
                                                      sushi_rpc::CpuTimings* response)
{
    auto [status, timings] = _controller->get_processor_timings(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    to_grpc(*response, timings);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::ResetAllTimings(grpc::ServerContext* /*context*/,
                                                  const sushi_rpc::GenericVoidValue* /*request*/,
                                                  sushi_rpc::GenericVoidValue* /*response*/)
{
    _controller->reset_all_timings();
    return grpc::Status::OK;
}

grpc::Status SushiControlService::ResetTrackTimings(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::TrackIdentifier* request,
                                                    sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->reset_track_timings(request->id());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::ResetProcessorTimings(grpc::ServerContext* /*context*/,
                                                        const sushi_rpc::ProcessorIdentifier* request,
                                                        sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->reset_processor_timings(request->id());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetTrackId(grpc::ServerContext* /*context*/,
                                             const sushi_rpc::GenericStringValue* request,
                                             sushi_rpc::TrackIdentifier* response)
{
    auto [status, id] = _controller->get_track_id(request->value());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status, "No track with that name");
    }
    response->set_id(id);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetTrackInfo(grpc::ServerContext* /*context*/,
                                               const sushi_rpc::TrackIdentifier* request,
                                               sushi_rpc::TrackInfo* response)
{
    auto [status, track] = _controller->get_track_info(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status, nullptr);
    }
    to_grpc(*response, track);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetTrackProcessors(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::TrackIdentifier* request,
                                                     sushi_rpc::ProcessorInfoList* response)
{
    auto [status, processors]= _controller->get_track_processors(request->id());
    for (const auto& processor : processors)
    {
        auto info = response->add_processors();
        to_grpc(*info, processor);
    }
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetTrackParameters(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::TrackIdentifier* request,
                                                     sushi_rpc::ParameterInfoList* response)
{
    auto [status, parameters] = _controller->get_track_parameters(request->id());
    for (const auto& parameter : parameters)
    {
        auto info = response->add_parameters();
        to_grpc(*info, parameter);
    }
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetProcessorId(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::GenericStringValue* request,
                                                 sushi_rpc::ProcessorIdentifier* response)
{
    auto [status, id] = _controller->get_processor_id(request->value());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status, "No processor with that name");
    }
    response->set_id(id);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorInfo(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::ProcessorIdentifier* request,
                                                   sushi_rpc::ProcessorInfo* response)
{
    auto [status, processor] = _controller->get_processor_info(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    to_grpc(*response, processor);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorBypassState(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::ProcessorIdentifier* request,
                                                          sushi_rpc::GenericBoolValue* response)
{
    auto [status, state] = _controller->get_processor_bypass_state(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(state);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetProcessorBypassState(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::ProcessorBypassStateSetRequest* request,
                                                          sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_processor_bypass_state(request->processor().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetProcessorCurrentProgram(grpc::ServerContext* /*context*/,
                                                             const sushi_rpc::ProcessorIdentifier* request,
                                                             sushi_rpc::ProgramIdentifier* response)
{
    auto [status, program] = _controller->get_processor_current_program(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_program(program);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorCurrentProgramName(grpc::ServerContext* /*context*/,
                                                                 const sushi_rpc::ProcessorIdentifier* request,
                                                                 sushi_rpc::GenericStringValue* response)
{
    auto [status, program] = _controller->get_processor_current_program_name(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(program);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorProgramName(grpc::ServerContext* /*context*/,
                                                          const sushi_rpc::ProcessorProgramIdentifier* request,
                                                          sushi_rpc::GenericStringValue* response)
{
    auto [status, program] = _controller->get_processor_program_name(request->processor().id(), request->program());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(program);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorPrograms(grpc::ServerContext* /*context*/,
                                                       const sushi_rpc::ProcessorIdentifier* request,
                                                       sushi_rpc::ProgramInfoList* response)
{
    auto [status, programs] = _controller->get_processor_programs(request->id());
    int id = 0;
    for (auto& program : programs)
    {
        auto info = response->add_programs();
        info->set_name(program);
        info->mutable_id()->set_program(id++);
    }
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SetProcessorProgram(grpc::ServerContext* /*context*/,
                                                      const sushi_rpc::ProcessorProgramSetRequest* request,
                                                      sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_processor_program(request->processor().id(), request->program().program());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetProcessorParameters(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ProcessorIdentifier* request,
                                                         sushi_rpc::ParameterInfoList* response)
{
    auto [status, parameters] = _controller->get_processor_parameters(request->id());
    for (const auto& parameter : parameters)
    {
        auto info = response->add_parameters();
        to_grpc(*info, parameter);
    }
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetParameterId(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::ParameterIdRequest* request,
                                                 sushi_rpc::ParameterIdentifier* response)
{
    auto [status, id] = _controller->get_parameter_id(request->processor().id(), request->parametername());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status,  "No parameter with that name");
    }
    response->set_parameter_id(id);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetParameterInfo(::grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::ParameterIdentifier* request,
                                                   sushi_rpc::ParameterInfo* response)
{
    auto [status, parameter] = _controller->get_parameter_info(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    to_grpc(*response, parameter);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetParameterValue(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::ParameterIdentifier* request,
                                                    sushi_rpc::GenericFloatValue* response)
{
    auto [status, value] = _controller->get_parameter_value(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(value);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetParameterValueNormalised(grpc::ServerContext* /*context*/,
                                                              const sushi_rpc::ParameterIdentifier* request,
                                                              sushi_rpc::GenericFloatValue* response)
{
    auto [status, value] = _controller->get_parameter_value_normalised(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(value);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetParameterValueAsString(grpc::ServerContext* /*context*/,
                                                            const sushi_rpc::ParameterIdentifier* request,
                                                            sushi_rpc::GenericStringValue* response)
{
    auto [status, value] = _controller->get_parameter_value_as_string(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(value);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetStringPropertyValue(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::ParameterIdentifier* request,
                                                         sushi_rpc::GenericStringValue* response)
{
    auto [status, value] = _controller->get_string_property_value(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_value(value);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetParameterValue(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::ParameterSetRequest* request,
                                                    sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_parameter_value(request->parameter().processor_id(),
                                                   request->parameter().parameter_id(),
                                                   request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SetParameterValueNormalised(grpc::ServerContext* /*context*/,
                                                    const sushi_rpc::ParameterSetRequest* request,
                                                    sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_parameter_value_normalised(request->parameter().processor_id(),
                                                              request->parameter().parameter_id(),
                                                              request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SetStringPropertyValue(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::StringPropertySetRequest* request,
                                                         sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_string_property_value(request->property().processor_id(),
                                                         request->property().parameter_id(),
                                                         request->value());
    return to_grpc_status(status);
}

void SushiControlService::notification(const sushi::ext::ControlNotification* notification)
{
    if (notification->type() == sushi::ext::NotificationType::PARAMETER_CHANGE)
    {
        auto typed_notification = static_cast<const sushi::ext::ParameterChangeNotification*>(notification);
        auto notification_content = std::make_shared<ParameterSetRequest>();
        notification_content->set_value(typed_notification->value());
        notification_content->mutable_parameter()->set_parameter_id(typed_notification->parameter_id());
        notification_content->mutable_parameter()->set_processor_id(typed_notification->processor_id());

        std::scoped_lock lock(_subscriber_lock);
        for(auto& subscriber : _parameter_subscribers)
        {
            subscriber->push(notification_content);
        }
    }
}

void SushiControlService::subscribe_to_parameter_updates(SubscribeToParameterUpdatesCallData* subscriber)
     {
         std::scoped_lock lock(_subscriber_lock);
         _parameter_subscribers.push_back(subscriber);
     }

void SushiControlService::unsubscribe_from_parameter_updates(SubscribeToParameterUpdatesCallData* subscriber)
{
    std::scoped_lock lock(_subscriber_lock);
    _parameter_subscribers.erase(std::remove(_parameter_subscribers.begin(),
                                            _parameter_subscribers.end(),
                                            subscriber));
}

void SushiControlService::stop_all_call_data()
{
    for (auto& subscriber : _parameter_subscribers)
    {
        subscriber->stop();
        subscriber->proceed();
    }
}

} // sushi_rpc
