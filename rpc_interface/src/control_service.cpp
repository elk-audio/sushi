
#include "control_service.h"

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

inline grpc::Status to_grpc_status(sushi::ext::ControlStatus status, const char* error = nullptr)
{
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
    return grpc::Status();
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
    response->set_denomainator(ts.denominator);
    response->set_numerator(ts.numerator);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetTimeSignature(grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::TimeSignature* request,
                                                   sushi_rpc::GenericVoidValue* /*response*/)
{
    sushi::ext::TimeSignature ts;
    ts.numerator = request->numerator();
    ts.denominator = request->denomainator();
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
        info->set_id(track.id);
        info->set_label(track.label);
        info->set_name((track.name));
        info->set_output_channels(track.output_channels);
        info->set_output_busses(track.output_busses);
        info->set_input_channels(track.input_channels);
        info->set_input_busses(track.input_busses);
    }
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SendNoteOn(grpc::ServerContext* /*context*/,
                                             const sushi_rpc::NoteOnRequest*request,
                                             sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_on(request->track().id(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteOff(grpc::ServerContext* /*context*/,
                                              const sushi_rpc::NoteOffRequest* request,
                                              sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_off(request->track().id(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteAftertouch(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::NoteAftertouchRequest* request,
                                                     sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_aftertouch(request->track().id(), request->note(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendAftertouch(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_aftertouch(request->track().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendPitchBend(grpc::ServerContext* /*context*/,
                                                const sushi_rpc::NoteModulationRequest* request,
                                                sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_pitch_bend(request->track().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendModulation(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_modulation(request->track().id(), request->value());
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
    response->set_average(timings.avg);
    response->set_min(timings.min);
    response->set_max(timings.max);
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
    response->set_average(timings.avg);
    response->set_min(timings.min);
    response->set_max(timings.max);
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
    response->set_average(timings.avg);
    response->set_min(timings.min);
    response->set_max(timings.max);
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
    auto [status, info] = _controller->get_track_info(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status, nullptr);
    }
    response->set_id(info.id);
    response->set_label(info.label);
    response->set_name((info.name));
    response->set_output_channels(info.output_channels);
    response->set_output_busses(info.output_busses);
    response->set_input_channels(info.input_channels);
    response->set_input_busses(info.input_busses);
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
        info->set_id(processor.id);
        info->set_label(processor.label);
        info->set_name((processor.name));
        info->set_parameter_count(processor.parameter_count);
    }
    return to_grpc_status(status, nullptr);
}

grpc::Status SushiControlService::GetTrackParameters(grpc::ServerContext* /*context*/,
                                                     const sushi_rpc::TrackIdentifier* request,
                                                     sushi_rpc::ParameterInfoList* response)
{
    auto [status, parameters] = _controller->get_track_parameters(request->id());
    for (const auto& parameter : parameters)
    {
        auto info = response->add_parameters();
        info->set_id(parameter.id);
        info->set_label(parameter.label);
        info->set_name(parameter.name);
        info->mutable_type()->set_type(to_grpc(parameter.type));
        info->set_min_range(parameter.min_range);
        info->set_max_range(parameter.max_range);
    }
    return to_grpc_status(status, nullptr);
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
    auto [status, info] = _controller->get_processor_info(request->id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_id(info.id);
    response->set_label(info.label);
    response->set_name((info.name));
    response->set_parameter_count(info.parameter_count);
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
    auto status = _controller->set_processor_bypass_state(request->processor().id(), true);
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
        info->set_id(parameter.id);
        info->set_label(parameter.label);
        info->set_name(parameter.name);
        info->mutable_type()->set_type(to_grpc(parameter.type));
        info->set_min_range(parameter.min_range);
        info->set_max_range(parameter.max_range);
    }
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetParameterId(grpc::ServerContext* /*context*/,
                                                 const sushi_rpc::ParameterIdRequest* request,
                                                 sushi_rpc::ParameterIdentifier* response)
{
    auto [status, id] = _controller->get_parameter_id(100, request->parametername());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status,  "No processor with that name");
    }
    response->set_parameter_id(id);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetParameterInfo(::grpc::ServerContext* /*context*/,
                                                   const sushi_rpc::ParameterIdentifier* request,
                                                   sushi_rpc::ParameterInfo* response)
{
    auto [status, info] = _controller->get_parameter_info(request->processor_id(), request->parameter_id());
    if (status != sushi::ext::ControlStatus::OK)
    {
        return to_grpc_status(status);
    }
    response->set_id(info.id);
    response->set_label(info.label);
    response->set_name(info.name);
    response->mutable_type()->set_type(to_grpc(info.type));
    response->set_min_range(info.min_range);
    response->set_max_range(info.max_range);
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

grpc::Status SushiControlService::SetStringPropertyValue(grpc::ServerContext* /*context*/,
                                                         const sushi_rpc::StringPropertySetRequest* request,
                                                         sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_string_property_value(request->property().processor_id(),
                                                         request->property().parameter_id(),
                                                         request->value());
    return to_grpc_status(status);
}

} // sushi_rpc