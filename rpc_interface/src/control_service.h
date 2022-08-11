/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SUSHICONTROLSERVICE_H
#define SUSHI_SUSHICONTROLSERVICE_H

#include <atomic>
#include <grpcpp/grpcpp.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sushi_rpc.grpc.pb.h"
#pragma GCC diagnostic pop

#include "control_interface.h"

namespace sushi_rpc {

class SubscribeToTransportChangesCallData;
class SubscribeToCpuTimingUpdatesCallData;
class SubscribeToTrackChangesCallData;
class SubscribeToProcessorChangesCallData;
class SubscribeToParameterUpdatesCallData;
class SubscribeToPropertyUpdatesCallData;

class SystemControlService : public SystemController::Service
{
public:
    SystemControlService(sushi::ext::SushiControl* controller) : _controller{controller->system_controller()} {}

    grpc::Status GetSushiVersion(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status GetBuildInfo(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::SushiBuildInfo* response) override;
    grpc::Status GetInputAudioChannelCount(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetOutputAudioChannelCount(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;

private:
    sushi::ext::SystemController* _controller;
};

class TransportControlService : public TransportController::Service
{
public:
    TransportControlService(sushi::ext::SushiControl* controller) : _controller{controller->transport_controller()} {}

    grpc::Status GetSamplerate(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericFloatValue* response) override;
    grpc::Status GetPlayingMode(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::PlayingMode* response) override;
    grpc::Status GetSyncMode(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::SyncMode* response) override;
    grpc::Status GetTimeSignature(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::TimeSignature* response) override;
    grpc::Status GetTempo(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericFloatValue* response) override;
    grpc::Status SetTempo(grpc::ServerContext* context, const sushi_rpc::GenericFloatValue* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SetPlayingMode(grpc::ServerContext* context, const sushi_rpc::PlayingMode* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SetSyncMode(grpc::ServerContext* context, const sushi_rpc::SyncMode* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SetTimeSignature(grpc::ServerContext* context, const sushi_rpc::TimeSignature* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::TransportController* _controller;
};

class TimingControlService : public TimingController::Service
{
public:
    TimingControlService(sushi::ext::SushiControl* controller) : _controller{controller->timing_controller()} {}

    grpc::Status GetTimingsEnabled(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericBoolValue* response) override;
    grpc::Status SetTimingsEnabled(grpc::ServerContext* context, const sushi_rpc::GenericBoolValue* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status GetEngineTimings(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::CpuTimings* response) override;
    grpc::Status GetTrackTimings(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::CpuTimings* response) override;
    grpc::Status GetProcessorTimings(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::CpuTimings* response) override;
    grpc::Status ResetAllTimings(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ResetTrackTimings(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ResetProcessorTimings(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::TimingController* _controller;
};

class KeyboardControlService : public KeyboardController::Service
{
public:
    KeyboardControlService(sushi::ext::SushiControl* controller) : _controller{controller->keyboard_controller()} {}

    grpc::Status SendNoteOn(grpc::ServerContext* context, const sushi_rpc::NoteOnRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SendNoteOff(grpc::ServerContext* context, const sushi_rpc::NoteOffRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SendNoteAftertouch(grpc::ServerContext* context, const sushi_rpc::NoteAftertouchRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SendAftertouch(grpc::ServerContext* context, const sushi_rpc::NoteModulationRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SendPitchBend(grpc::ServerContext* context, const sushi_rpc::NoteModulationRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SendModulation(grpc::ServerContext* context, const sushi_rpc::NoteModulationRequest* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::KeyboardController* _controller;
};

class AudioGraphControlService : public AudioGraphController::Service
{
public:
    AudioGraphControlService(sushi::ext::SushiControl* controller) : _controller{controller->audio_graph_controller()} {}

    grpc::Status GetAllProcessors(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::ProcessorInfoList* response) override;
    grpc::Status GetAllTracks(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::TrackInfoList* response) override;
    grpc::Status GetTrackId(grpc::ServerContext* context, const sushi_rpc::GenericStringValue* request, sushi_rpc::TrackIdentifier* response) override;
    grpc::Status GetTrackInfo(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::TrackInfo* response) override;
    grpc::Status GetTrackProcessors(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::ProcessorInfoList* response) override;
    grpc::Status GetProcessorId(grpc::ServerContext* context, const sushi_rpc::GenericStringValue* request, sushi_rpc::ProcessorIdentifier* response) override;
    grpc::Status GetProcessorInfo(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::ProcessorInfo* response) override;
    grpc::Status GetProcessorBypassState(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericBoolValue* response) override;
    grpc::Status GetProcessorState(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::ProcessorState* response) override;
    grpc::Status SetProcessorBypassState(grpc::ServerContext* context, const sushi_rpc::ProcessorBypassStateSetRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status SetProcessorState(grpc::ServerContext* context, const sushi_rpc::ProcessorStateSetRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status CreateTrack(grpc::ServerContext* context, const sushi_rpc::CreateTrackRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status CreateMultibusTrack(grpc::ServerContext* context, const sushi_rpc::CreateMultibusTrackRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status CreatePreTrack(grpc::ServerContext* context, const sushi_rpc::CreatePreTrackRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status CreatePostTrack(grpc::ServerContext* context, const sushi_rpc::CreatePostTrackRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status CreateProcessorOnTrack(grpc::ServerContext* context, const sushi_rpc::CreateProcessorRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status MoveProcessorOnTrack(grpc::ServerContext* context, const sushi_rpc::MoveProcessorRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DeleteProcessorFromTrack(grpc::ServerContext* context, const sushi_rpc::DeleteProcessorRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DeleteTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::AudioGraphController* _controller;
};

class ParameterControlService : public ParameterController::Service
{
public:
    ParameterControlService(sushi::ext::SushiControl* controller) : _controller{controller->parameter_controller()} {}

    grpc::Status GetTrackParameters(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::ParameterInfoList* response) override;
    grpc::Status GetProcessorParameters(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::ParameterInfoList* response) override;
    grpc::Status GetParameterId(grpc::ServerContext* context, const sushi_rpc::ParameterIdRequest* request, sushi_rpc::ParameterIdentifier* response) override;
    grpc::Status GetParameterInfo(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::ParameterInfo* response) override;
    grpc::Status GetParameterValue(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::GenericFloatValue* response) override;
    grpc::Status GetParameterValueInDomain(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::GenericFloatValue* response) override;
    grpc::Status GetParameterValueAsString(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status SetParameterValue(grpc::ServerContext* context, const sushi_rpc::ParameterValue* request, sushi_rpc::GenericVoidValue* response) override;

    grpc::Status GetTrackProperties(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::PropertyInfoList* response) override;
    grpc::Status GetProcessorProperties(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::PropertyInfoList* response) override;
    grpc::Status GetPropertyId(grpc::ServerContext* context, const sushi_rpc::PropertyIdRequest* request, sushi_rpc::PropertyIdentifier* response) override;
    grpc::Status GetPropertyInfo(grpc::ServerContext* context, const sushi_rpc::PropertyIdentifier* request, sushi_rpc::PropertyInfo* response) override;
    grpc::Status GetPropertyValue(grpc::ServerContext* context, const sushi_rpc::PropertyIdentifier* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status SetPropertyValue(grpc::ServerContext* context, const sushi_rpc::PropertyValue* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::ParameterController* _controller;
};

class ProgramControlService : public ProgramController::Service
{
public:
    ProgramControlService(sushi::ext::SushiControl* controller) : _controller{controller->program_controller()} {}

    grpc::Status GetProcessorCurrentProgram(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::ProgramIdentifier* response) override;
    grpc::Status GetProcessorCurrentProgramName(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status GetProcessorProgramName(grpc::ServerContext* context, const sushi_rpc::ProcessorProgramIdentifier* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status GetProcessorPrograms(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::ProgramInfoList* response) override;
    grpc::Status SetProcessorProgram(grpc::ServerContext* context, const sushi_rpc::ProcessorProgramSetRequest* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::ProgramController* _controller;
};

class MidiControlService : public MidiController::Service
{
public:
    MidiControlService(sushi::ext::SushiControl* controller) : _midi_controller{controller->midi_controller()} {}

    grpc::Status GetInputPorts(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetOutputPorts(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetAllKbdInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::MidiKbdConnectionList* response) override;
    grpc::Status GetAllKbdOutputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::MidiKbdConnectionList* response) override;
    grpc::Status GetAllCCInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::MidiCCConnectionList* response) override;
    grpc::Status GetAllPCInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::MidiPCConnectionList* response) override;
    grpc::Status GetCCInputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::MidiCCConnectionList* response) override;
    grpc::Status GetPCInputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::MidiPCConnectionList* response) override;
    grpc::Status GetMidiClockOutputEnabled(grpc::ServerContext* context, const sushi_rpc::GenericIntValue* request, sushi_rpc::GenericBoolValue* response) override;
    grpc::Status SetMidiClockOutputEnabled(grpc::ServerContext* context, const sushi_rpc::MidiClockSetRequest* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectKbdInputToTrack(grpc::ServerContext* context, const sushi_rpc::MidiKbdConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectKbdOutputFromTrack(grpc::ServerContext* context, const sushi_rpc::MidiKbdConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectCCToParameter(grpc::ServerContext* context, const sushi_rpc::MidiCCConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectPCToProcessor(grpc::ServerContext* context, const sushi_rpc::MidiPCConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectKbdInput(grpc::ServerContext* context, const sushi_rpc::MidiKbdConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectKbdOutput(grpc::ServerContext* context, const sushi_rpc::MidiKbdConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectCC(grpc::ServerContext* context, const sushi_rpc::MidiCCConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectPC(grpc::ServerContext* context, const sushi_rpc::MidiPCConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllCCFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllPCFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::MidiController* _midi_controller;
};

class AudioRoutingControlService : public AudioRoutingController::Service
{
public:
    AudioRoutingControlService(sushi::ext::SushiControl* controller) : _controller{controller->audio_routing_controller()} {}

    grpc::Status GetAllInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::AudioConnectionList* response) override;
    grpc::Status GetAllOutputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::AudioConnectionList* response) override;
    grpc::Status GetInputConnectionsForTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::AudioConnectionList* response) override;
    grpc::Status GetOutputConnectionsForTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::AudioConnectionList* response) override;
    grpc::Status ConnectInputChannelToTrack(grpc::ServerContext* context, const sushi_rpc::AudioConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectOutputChannelFromTrack(grpc::ServerContext* context, const sushi_rpc::AudioConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectInput(grpc::ServerContext* context, const sushi_rpc::AudioConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectOutput(grpc::ServerContext* context, const sushi_rpc::AudioConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllInputsFromTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllOutputFromTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllOutputsFromTrack(grpc::ServerContext* context, const sushi_rpc::TrackIdentifier* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::AudioRoutingController* _controller;
};

class CvGateControlService : public CvGateController::Service
{
public:
    CvGateControlService(sushi::ext::SushiControl* controller) : _controller{controller->cv_gate_controller()} {}

    grpc::Status GetCvInputChannelCount(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetCvOutputChannelCount(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetAllCvInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::CvConnectionList* response) override;
    grpc::Status GetAllCvOutputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::CvConnectionList* response) override;
    grpc::Status GetAllGateInputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GateConnectionList* response) override;
    grpc::Status GetAllGateOutputConnections(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GateConnectionList* response) override;
    grpc::Status GetCvInputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::CvConnectionList* response) override;
    grpc::Status GetCvOutputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::CvConnectionList* response) override;
    grpc::Status GetGateInputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GateConnectionList* response) override;
    grpc::Status GetGateOutputConnectionsForProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GateConnectionList* response) override;
    grpc::Status ConnectCvInputToParameter(grpc::ServerContext* context, const sushi_rpc::CvConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectCvOutputFromParameter(grpc::ServerContext* context, const sushi_rpc::CvConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectGateInputToProcessor(grpc::ServerContext* context, const sushi_rpc::GateConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status ConnectGateOutputFromProcessor(grpc::ServerContext* context, const sushi_rpc::GateConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectCvInput(grpc::ServerContext* context, const sushi_rpc::CvConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectCvOutput(grpc::ServerContext* context, const sushi_rpc::CvConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectGateInput(grpc::ServerContext* context, const sushi_rpc::GateConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectGateOutput(grpc::ServerContext* context, const sushi_rpc::GateConnection* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllCvInputsFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllCvOutputsFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllGateInputsFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisconnectAllGateOutputsFromProcessor(grpc::ServerContext* context, const sushi_rpc::ProcessorIdentifier* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::CvGateController* _controller;
};

class OscControlService : public OscController::Service
{
public:
    OscControlService(sushi::ext::SushiControl* controller) : _controller{controller->osc_controller()} {}

    grpc::Status GetSendIP(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericStringValue* response) override;
    grpc::Status GetSendPort(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetReceivePort(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericIntValue* response) override;
    grpc::Status GetEnabledParameterOutputs(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::OscParameterOutputList* response) override;
    grpc::Status EnableOutputForParameter(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisableOutputForParameter(grpc::ServerContext* context, const sushi_rpc::ParameterIdentifier* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status EnableAllOutput(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericVoidValue* response) override;
    grpc::Status DisableAllOutput(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::OscController* _controller;
};

class SessionControlService : public SessionController::Service
{
public:
    SessionControlService(sushi::ext::SushiControl* controller) : _controller{controller->session_controller()} {}

    grpc::Status SaveSession(grpc::ServerContext* context, const sushi_rpc::GenericVoidValue* request, sushi_rpc::SessionState* response) override;
    grpc::Status RestoreSession(grpc::ServerContext* context, const sushi_rpc::SessionState* request, sushi_rpc::GenericVoidValue* response) override;

private:
    sushi::ext::SessionController* _controller;
};

using AsyncService = sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToParameterUpdates<
                     sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToProcessorChanges<
                     sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToTrackChanges<
                     sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToEngineCpuTimingUpdates<
                     sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToTransportChanges<
                     sushi_rpc::NotificationController::WithAsyncMethod_SubscribeToPropertyUpdates<
                     sushi_rpc::NotificationController::Service
                     >>>>>>;

class NotificationControlService : public AsyncService,
                                   private sushi::ext::ControlListener
{
public:
    NotificationControlService(sushi::ext::SushiControl* controller);

    // Inherited from ControlListener
    void notification(const sushi::ext::ControlNotification* notification) override;

    void subscribe(SubscribeToTransportChangesCallData* subscriber);
    void unsubscribe(SubscribeToTransportChangesCallData* subscriber);

    void subscribe(SubscribeToCpuTimingUpdatesCallData* subscriber);
    void unsubscribe(SubscribeToCpuTimingUpdatesCallData* subscriber);

    void subscribe(SubscribeToTrackChangesCallData* subscriber);
    void unsubscribe(SubscribeToTrackChangesCallData* subscriber);

    void subscribe(SubscribeToProcessorChangesCallData* subscriber);
    void unsubscribe(SubscribeToProcessorChangesCallData* subscriber);

    void subscribe(SubscribeToParameterUpdatesCallData* subscriber);
    void unsubscribe(SubscribeToParameterUpdatesCallData* subscriber);

    void subscribe(SubscribeToPropertyUpdatesCallData* subscriber);
    void unsubscribe(SubscribeToPropertyUpdatesCallData* subscriber);

    void delete_all_subscribers();

private:
    void _forward_transport_notification_to_subscribers(const sushi::ext::ControlNotification* notification);
    void _forward_cpu_timing_notification_to_subscribers(const sushi::ext::ControlNotification* notification);
    void _forward_track_notification_to_subscribers(const sushi::ext::ControlNotification* notification);
    void _forward_processor_notification_to_subscribers(const sushi::ext::ControlNotification* notification);
    void _forward_parameter_notification_to_subscribers(const sushi::ext::ControlNotification* notification);
    void _forward_property_notification_to_subscribers(const sushi::ext::ControlNotification* notification);

    std::vector<SubscribeToTransportChangesCallData*> _transport_subscribers;
    std::mutex _transport_subscriber_lock;

    std::vector<SubscribeToCpuTimingUpdatesCallData*> _timing_subscribers;
    std::mutex _timing_subscriber_lock;

    std::vector<SubscribeToTrackChangesCallData*> _track_subscribers;
    std::mutex _track_subscriber_lock;

    std::vector<SubscribeToProcessorChangesCallData*> _processor_subscribers;
    std::mutex _processor_subscriber_lock;

    std::vector<SubscribeToParameterUpdatesCallData*> _parameter_subscribers;
    std::mutex _parameter_subscriber_lock;

    std::vector<SubscribeToPropertyUpdatesCallData*> _property_subscribers;
    std::mutex _property_subscriber_lock;

    sushi::ext::SushiControl* _controller;

    sushi::ext::AudioGraphController* _audio_graph_controller;
};

}// sushi_rpc

#endif //SUSHI_SUSHICONTROLSERVICE_H
