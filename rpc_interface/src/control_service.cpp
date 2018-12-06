
#include "control_service.h"

namespace sushi_rpc {

::grpc::Status to_grpc_status(sushi::ext::ControlStatus status, const char* error = nullptr)
{
    switch (status)
    {
        case sushi::ext::ControlStatus::OK:
            return ::grpc::Status::OK;

        case sushi::ext::ControlStatus::ERROR:
            return ::grpc::Status(::grpc::StatusCode::UNKNOWN, error);

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

grpc::Status
SushiControlService::GetSamplerate(::grpc::ServerContext* /*context*/, const ::sushi_rpc::GenericVoidValue*request,
                                   sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_samplerate());
    return grpc::Status::OK;
}

::grpc::Status
SushiControlService::GetPlayingMode(::grpc::ServerContext* /*context*/, const ::sushi_rpc::GenericVoidValue*request,
                                    ::sushi_rpc::PlayingMode* response)
{
    auto mode = _controller->get_playing_mode();
    switch (mode)
    {
        case sushi::ext::PlayingMode::STOPPED:
            response->set_mode(PlayingMode_Mode_STOPPED);
            break;

        case sushi::ext::PlayingMode::PLAYING:
            response->set_mode(PlayingMode_Mode_PLAYING);
            break;

        case sushi::ext::PlayingMode::RECORDING:
            response->set_mode(PlayingMode_Mode_RECORDING);
            break;
    }
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetPlayingMode(grpc::ServerContext* /*context*/,
                                                 const ::sushi_rpc::PlayingMode* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    switch (request->mode())
    {
        case sushi_rpc::PlayingMode::STOPPED:
            _controller->set_playing_mode(sushi::ext::PlayingMode::STOPPED);
            break;

        case sushi_rpc::PlayingMode::PLAYING:
            _controller->set_playing_mode(sushi::ext::PlayingMode::PLAYING);
            break;

        case sushi_rpc::PlayingMode::RECORDING:
            _controller->set_playing_mode(sushi::ext::PlayingMode::RECORDING);
            break;

        default:
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, nullptr);
    }
    return grpc::Status();
}

grpc::Status SushiControlService::GetSyncMode(grpc::ServerContext* /*context*/,
                                              const ::sushi_rpc::GenericVoidValue*request,
                                              sushi_rpc::SyncMode* response)
{
    auto mode = _controller->get_sync_mode();
    switch (mode)
    {
        case sushi::ext::SyncMode::INTERNAL:
            response->set_mode(SyncMode_Mode_INTERNAL);
            break;

        case sushi::ext::SyncMode::MIDI:
            response->set_mode(SyncMode_Mode_MIDI);
            break;

        case sushi::ext::SyncMode::LINK:
            response->set_mode(SyncMode_Mode_LINK);
            break;
    }
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetSyncMode(grpc::ServerContext* /*context*/,
                                              const ::sushi_rpc::SyncMode*request,
                                              sushi_rpc::GenericVoidValue* /*response*/)
{
    switch (request->mode())
    {
        case sushi_rpc::SyncMode::INTERNAL:
            _controller->set_sync_mode(sushi::ext::SyncMode::INTERNAL);
            break;

        case sushi_rpc::SyncMode::MIDI:
            _controller->set_sync_mode(sushi::ext::SyncMode::MIDI);
            break;

        case sushi_rpc::SyncMode::LINK:
            _controller->set_sync_mode(sushi::ext::SyncMode::LINK);
            break;

        default:
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, nullptr);
    }
    return grpc::Status();
}

grpc::Status SushiControlService::GetTempo(grpc::ServerContext* /*context*/,
                                           const ::sushi_rpc::GenericVoidValue* /*request*/,
                                           sushi_rpc::GenericFloatValue* response)
{
    response->set_value(_controller->get_tempo());
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetTempo(grpc::ServerContext* /*context*/,
                                           const ::sushi_rpc::GenericFloatValue* request,
                                           sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->set_tempo(request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetTimeSignature(grpc::ServerContext* /*context*/,
                                                   const ::sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::TimeSignature* response)
{
    auto ts = _controller->get_time_signature();
    response->set_denomainator(ts.denominator);
    response->set_numerator(ts.numerator);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::SetTimeSignature(grpc::ServerContext* /*context*/,
                                                   const ::sushi_rpc::TimeSignature* request,
                                                   sushi_rpc::GenericVoidValue* /*response*/)
{
    sushi::ext::TimeSignature ts;
    ts.numerator = request->numerator();
    ts.denominator = request->denomainator();
    auto status = _controller->set_time_signature(ts);
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteOn(grpc::ServerContext* /*context*/,
                                             const ::sushi_rpc::NoteOnRequest*request,
                                             sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_on(request->track().id(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteOff(grpc::ServerContext* /*context*/,
                                              const ::sushi_rpc::NoteOffRequest* request,
                                              sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_off(request->track().id(), request->note(), request->velocity());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendNoteAftertouch(grpc::ServerContext* /*context*/,
                                                     const ::sushi_rpc::NoteAftertouchRequest* request,
                                                     sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_note_aftertouch(request->track().id(), request->note(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendAftertouch(grpc::ServerContext* /*context*/,
                                                 const ::sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_aftertouch(request->track().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendPitchBend(grpc::ServerContext* /*context*/,
                                                const ::sushi_rpc::NoteModulationRequest* request,
                                                sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_pitch_bend(request->track().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::SendModulation(grpc::ServerContext* /*context*/,
                                                 const ::sushi_rpc::NoteModulationRequest* request,
                                                 sushi_rpc::GenericVoidValue* /*response*/)
{
    auto status = _controller->send_modulation(request->track().id(), request->value());
    return to_grpc_status(status);
}

grpc::Status SushiControlService::GetEngineTimings(grpc::ServerContext* /*context*/,
                                                   const ::sushi_rpc::GenericVoidValue* /*request*/,
                                                   sushi_rpc::CpuTimings* response)
{
    auto timings = _controller->get_engine_timings();
    response->set_average(timings.avg);
    response->set_min(timings.min);
    response->set_max(timings.max);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetTrackTimings(grpc::ServerContext* /*context*/,
                                                  const ::sushi_rpc::TrackIdentifier* request,
                                                  sushi_rpc::CpuTimings* response)
{
    auto timings = _controller->get_track_timings(request->id());
    if (!timings.has_value())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Invalid Track Id");
    }
    auto t = timings.value();
    response->set_average(t.avg);
    response->set_min(t.min);
    response->set_max(t.max);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::GetProcessorTimings(grpc::ServerContext* /*context*/,
                                                      const ::sushi_rpc::ProcessorIdentifier* request,
                                                      sushi_rpc::CpuTimings* response)
{
    auto timings = _controller->get_processor_timings(request->id());
    if (!timings.has_value())
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Invalid Processor Id");
    }
    auto t = timings.value();
    response->set_average(t.avg);
    response->set_min(t.min);
    response->set_max(t.max);
    return grpc::Status::OK;
}

grpc::Status SushiControlService::ResetAllTimings(grpc::ServerContext* /*context*/,
                                                  const ::sushi_rpc::GenericVoidValue* /*request*/,
                                                  sushi_rpc::GenericVoidValue* /*response*/)
{
    _controller->reset_all_timings();
    return grpc::Status::OK;
}

::grpc::Status
SushiControlService::ResetTrackTimings(::grpc::ServerContext* /*context*/, const ::sushi_rpc::TrackIdentifier*request,
                                       ::sushi_rpc::GenericVoidValue* /*response*/)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::ResetProcessorTimings(::grpc::ServerContext* /*context*/,
                                                          const ::sushi_rpc::ProcessorIdentifier*request,
                                                          ::sushi_rpc::GenericVoidValue* /*response*/)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetTrackId(::grpc::ServerContext* /*context*/, const ::sushi_rpc::GenericStringValue*request,
                                ::sushi_rpc::TrackIdentifier* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetTrackLabel(::grpc::ServerContext* /*context*/, const ::sushi_rpc::TrackIdentifier*request,
                                   ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetTrackName(::grpc::ServerContext* /*context*/, const ::sushi_rpc::TrackIdentifier*request,
                                  ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetTrackInfo(::grpc::ServerContext* /*context*/, const ::sushi_rpc::TrackIdentifier*request,
                                  ::sushi_rpc::TrackInfo* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetProcessorId(::grpc::ServerContext* /*context*/, const ::sushi_rpc::GenericStringValue*request,
                                    ::sushi_rpc::ProcessorIdentifier* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetProcessorLabel(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ProcessorIdentifier*request,
                                       ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetProcessorName(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ProcessorIdentifier*request,
                                      ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetProcessorInfo(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ProcessorIdentifier*request,
                                      ::sushi_rpc::TrackInfo* response)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::GetProcessorBypassState(::grpc::ServerContext* /*context*/,
                                                            const ::sushi_rpc::ProcessorIdentifier*request,
                                                            ::sushi_rpc::GenericBoolValue* response)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::SetProcessorBypassState(::grpc::ServerContext* /*context*/,
                                                            const ::sushi_rpc::ProcessorIdentifier*request,
                                                            ::sushi_rpc::GenericVoidValue* /*response*/)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetParameterId(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ParameterIdRequest*request,
                                    ::sushi_rpc::ParameterIdentifier* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetParameterLabel(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ParameterIdentifier*request,
                                       ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetParameterName(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ParameterIdentifier*request,
                                      ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetParameterInfo(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ParameterIdentifier*request,
                                      ::sushi_rpc::ParameterInfo* response)
{
    return grpc::Status();
}

::grpc::Status
SushiControlService::GetParameterValue(::grpc::ServerContext* /*context*/, const ::sushi_rpc::ParameterIdentifier*request,
                                       ::sushi_rpc::GenericFloatValue* response)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::GetParameterValueAsString(::grpc::ServerContext* /*context*/,
                                                              const ::sushi_rpc::ParameterIdentifier*request,
                                                              ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::GetStringPropertyValue(::grpc::ServerContext* /*context*/,
                                                           const ::sushi_rpc::ParameterIdentifier*request,
                                                           ::sushi_rpc::GenericStringValue* response)
{
    return grpc::Status();
}

::grpc::Status SushiControlService::SetStringPropertyValue(::grpc::ServerContext* /*context*/,
                                                           const ::sushi_rpc::ParameterIdentifier*request,
                                                           ::sushi_rpc::GenericFloatValue* response)
{
    return grpc::Status();
}
} // sushi_rpc