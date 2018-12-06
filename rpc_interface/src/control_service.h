/**
 * @brief Sushi Control Service, gRPC service for external control of Sushi
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SUSHICONTROLSERVICE_H
#define SUSHI_SUSHICONTROLSERVICE_H

#include <grpc++/grpc++.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sushi_rpc.grpc.pb.h"
#pragma GCC diagnostic pop

#include "../../include/control_interface.h"

namespace sushi_rpc {

class SushiControlService : public sushi_rpc::SushiController::Service
{
public:
    SushiControlService(sushi::ext::SushiControl* controller) : _controller{controller} {};

    virtual ~SushiControlService() = default;

    // Engine control
     ::grpc::Status GetSamplerate(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::GenericFloatValue* response) override;
     ::grpc::Status GetPlayingMode(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::PlayingMode* response) override;
     ::grpc::Status SetPlayingMode(::grpc::ServerContext* context, const ::sushi_rpc::PlayingMode* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status GetSyncMode(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::SyncMode* response) override;
     ::grpc::Status SetSyncMode(::grpc::ServerContext* context, const ::sushi_rpc::SyncMode* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status GetTempo(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::GenericFloatValue* response) override;
     ::grpc::Status SetTempo(::grpc::ServerContext* context, const ::sushi_rpc::GenericFloatValue* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status GetTimeSignature(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::TimeSignature* response) override;
     ::grpc::Status SetTimeSignature(::grpc::ServerContext* context, const ::sushi_rpc::TimeSignature* request, ::sushi_rpc::GenericVoidValue* response) override;
    // Keyboard control
     ::grpc::Status SendNoteOn(::grpc::ServerContext* context, const ::sushi_rpc::NoteOnRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status SendNoteOff(::grpc::ServerContext* context, const ::sushi_rpc::NoteOffRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status SendNoteAftertouch(::grpc::ServerContext* context, const ::sushi_rpc::NoteAftertouchRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status SendAftertouch(::grpc::ServerContext* context, const ::sushi_rpc::NoteModulationRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status SendPitchBend(::grpc::ServerContext* context, const ::sushi_rpc::NoteModulationRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status SendModulation(::grpc::ServerContext* context, const ::sushi_rpc::NoteModulationRequest* request, ::sushi_rpc::GenericVoidValue* response) override;
    // Cpu timings
     ::grpc::Status GetEngineTimings(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::CpuTimings* response) override;
     ::grpc::Status GetTrackTimings(::grpc::ServerContext* context, const ::sushi_rpc::TrackIdentifier* request, ::sushi_rpc::CpuTimings* response) override;
     ::grpc::Status GetProcessorTimings(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::CpuTimings* response) override;
     ::grpc::Status ResetAllTimings(::grpc::ServerContext* context, const ::sushi_rpc::GenericVoidValue* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status ResetTrackTimings(::grpc::ServerContext* context, const ::sushi_rpc::TrackIdentifier* request, ::sushi_rpc::GenericVoidValue* response) override;
     ::grpc::Status ResetProcessorTimings(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::GenericVoidValue* response) override;
    // Track control
     ::grpc::Status GetTrackId(::grpc::ServerContext* context, const ::sushi_rpc::GenericStringValue* request, ::sushi_rpc::TrackIdentifier* response) override;
     ::grpc::Status GetTrackLabel(::grpc::ServerContext* context, const ::sushi_rpc::TrackIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetTrackName(::grpc::ServerContext* context, const ::sushi_rpc::TrackIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetTrackInfo(::grpc::ServerContext* context, const ::sushi_rpc::TrackIdentifier* request, ::sushi_rpc::TrackInfo* response) override;
    // Processor control
     ::grpc::Status GetProcessorId(::grpc::ServerContext* context, const ::sushi_rpc::GenericStringValue* request, ::sushi_rpc::ProcessorIdentifier* response) override;
     ::grpc::Status GetProcessorLabel(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetProcessorName(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetProcessorInfo(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::TrackInfo* response) override;
     ::grpc::Status GetProcessorBypassState(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::GenericBoolValue* response) override;
     ::grpc::Status SetProcessorBypassState(::grpc::ServerContext* context, const ::sushi_rpc::ProcessorIdentifier* request, ::sushi_rpc::GenericVoidValue* response) override;
    // Parameter control
     ::grpc::Status GetParameterId(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdRequest* request, ::sushi_rpc::ParameterIdentifier* response) override;
     ::grpc::Status GetParameterLabel(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetParameterName(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetParameterInfo(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::ParameterInfo* response) override;
     ::grpc::Status GetParameterValue(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericFloatValue* response) override;
     ::grpc::Status GetParameterValueAsString(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status GetStringPropertyValue(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericStringValue* response) override;
     ::grpc::Status SetStringPropertyValue(::grpc::ServerContext* context, const ::sushi_rpc::ParameterIdentifier* request, ::sushi_rpc::GenericFloatValue* response) override;

private:

    sushi::ext::SushiControl* _controller;
};

}// sushi_rpc

#endif //SUSHI_SUSHICONTROLSERVICE_H
