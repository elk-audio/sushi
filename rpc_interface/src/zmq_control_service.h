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
#ifndef SUSHI_LIBRARY_ZMQ_CONTROL_SERVICE_H
#define SUSHI_LIBRARY_ZMQ_CONTROL_SERVICE_H

#include <zmq.hpp>

#include "sushi_rpc.pb.h"
#include "sushi_rpc_zmq.h"

#include <grpcpp/impl/codegen/config_protobuf.h>

#include "sushi/control_interface.h"
#include "sushi/control_notifications.h"

namespace sushi_ipc{

class SystemControlService : public SystemControllerClient
{
public:
    SystemControlService(sushi::control::SushiControl* controller) : _controller(controller->system_controller()) {}

    ~SystemControlService() = default;

    void GetSushiVersion(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericStringValue& response) override;
    void GetSushiApiVersion(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericStringValue& response) override;
    void GetBuildInfo(const sushi_rpc::GenericVoidValue& request, sushi_rpc::SushiBuildInfo& response) override;
    void GetInputAudioChannelCount(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetOutputAudioChannelCount(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;

private:
    sushi::control::SystemController* _controller;
};

class TransportControlService : public TransportControllerClient
{
public:
    TransportControlService(sushi::control::SushiControl* controller) : _controller(controller->transport_controller()) {}

    ~TransportControlService() = default;

    void GetSamplerate(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericFloatValue& response) override;
    void GetPlayingMode(const sushi_rpc::GenericVoidValue& request, sushi_rpc::PlayingMode& response) override;
    void GetSyncMode(const sushi_rpc::GenericVoidValue& request, sushi_rpc::SyncMode& response) override;
    void GetTimeSignature(const sushi_rpc::GenericVoidValue& request, sushi_rpc::TimeSignature& response) override;
    void GetTempo(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericFloatValue& response) override;
    void SetTempo(const sushi_rpc::GenericFloatValue& request, sushi_rpc::CommandResponse& response) override;
    void SetPlayingMode(const sushi_rpc::PlayingMode& request, sushi_rpc::CommandResponse& response) override;
    void SetSyncMode(const sushi_rpc::SyncMode& request, sushi_rpc::CommandResponse& response) override;
    void SetTimeSignature(const sushi_rpc::TimeSignature& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::TransportController* _controller;
};

class TimingControlService : public TimingControllerClient
{
public:
    TimingControlService(sushi::control::SushiControl* controller) : _controller(controller->timing_controller()) {}

    ~TimingControlService() = default;

    void GetTimingsEnabled(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericBoolValue& response) override;
    void SetTimingsEnabled(const sushi_rpc::GenericBoolValue& request, sushi_rpc::CommandResponse& response) override;
    void GetEngineTimings(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CpuTimings& response) override;
    void GetTrackTimings(const sushi_rpc::TrackIdentifier& request, sushi_rpc::TimingResponse& response) override;
    void GetProcessorTimings(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::TimingResponse& response) override;
    void ResetAllTimings(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CommandResponse& response) override;
    void ResetTrackTimings(const sushi_rpc::TrackIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void ResetProcessorTimings(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::TimingController* _controller;
};

class KeyboardControlService : public KeyboardControllerClient
{
public:
    KeyboardControlService(sushi::control::SushiControl* controller) : _controller(controller->keyboard_controller()) {}

    ~KeyboardControlService() = default;

    void SendNoteOn(const sushi_rpc::NoteOnRequest& request, sushi_rpc::CommandResponse& response) override;
    void SendNoteOff(const sushi_rpc::NoteOffRequest& request, sushi_rpc::CommandResponse& response) override;
    void SendNoteAftertouch(const sushi_rpc::NoteAftertouchRequest& request, sushi_rpc::CommandResponse& response) override;
    void SendAftertouch(const sushi_rpc::NoteModulationRequest& request, sushi_rpc::CommandResponse& response) override;
    void SendPitchBend(const sushi_rpc::NoteModulationRequest& request, sushi_rpc::CommandResponse& response) override;
    void SendModulation(const sushi_rpc::NoteModulationRequest& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::KeyboardController* _controller;
};

class AudioGraphControlService : public AudioGraphControllerClient
{
public:
    AudioGraphControlService(sushi::control::SushiControl* controller) : _controller(controller->audio_graph_controller()) {}

    ~AudioGraphControlService() = default;

    void GetAllProcessors(const sushi_rpc::GenericVoidValue& request, sushi_rpc::ProcessorInfoList& response) override;
    void GetAllTracks(const sushi_rpc::GenericVoidValue& request, sushi_rpc::TrackInfoList& response) override;
    void GetTrackId(const sushi_rpc::GenericStringValue& request, sushi_rpc::TrackIdentifierResponse& response) override;
    void GetTrackInfo(const sushi_rpc::TrackIdentifier& request, sushi_rpc::TrackInfoResponse& response) override;
    void GetTrackProcessors(const sushi_rpc::TrackIdentifier& request, sushi_rpc::ProcessorInfoListResponse& response) override;
    void GetProcessorId(const sushi_rpc::GenericStringValue& request, sushi_rpc::ProcessorIdentifierResponse& response) override;
    void GetProcessorInfo(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::ProcessorInfoResponse& response) override;
    void GetProcessorBypassState(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::BoolResponse& response) override;
    void GetProcessorState(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::ProcessorStateResponse& response) override;
    void SetProcessorBypassState(const sushi_rpc::ProcessorBypassStateSetRequest& request, sushi_rpc::CommandResponse& response) override;
    void SetProcessorState(const sushi_rpc::ProcessorStateSetRequest& request, sushi_rpc::CommandResponse& response) override;
    void CreateTrack(const sushi_rpc::CreateTrackRequest& request, sushi_rpc::CommandResponse& response) override;
    void CreateMultibusTrack(const sushi_rpc::CreateMultibusTrackRequest& request, sushi_rpc::CommandResponse& response) override;
    void CreatePreTrack(const sushi_rpc::CreatePreTrackRequest& request, sushi_rpc::CommandResponse& response) override;
    void CreatePostTrack(const sushi_rpc::CreatePostTrackRequest& request, sushi_rpc::CommandResponse& response) override;
    void CreateProcessorOnTrack(const sushi_rpc::CreateProcessorRequest& request, sushi_rpc::CommandResponse& response) override;
    void MoveProcessorOnTrack(const sushi_rpc::MoveProcessorRequest& request, sushi_rpc::CommandResponse& response) override;
    void DeleteProcessorFromTrack(const sushi_rpc::DeleteProcessorRequest& request, sushi_rpc::CommandResponse& response) override;
    void DeleteTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::AudioGraphController* _controller;
};

class ProgramControlService : public ProgramControllerClient
{
public:
    ProgramControlService(sushi::control::SushiControl* controller) : _controller(controller->program_controller()) {}

    ~ProgramControlService() = default;

    void GetProcessorCurrentProgram(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::ProgramIdentifierResponse& response) override;
    void GetProcessorCurrentProgramName(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::StringResponse& response) override;
    void GetProcessorProgramName(const sushi_rpc::ProcessorProgramIdentifier& request, sushi_rpc::StringResponse& response) override;
    void GetProcessorPrograms(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::ProgramInfoListResponse& response) override;
    void SetProcessorProgram(const sushi_rpc::ProcessorProgramSetRequest& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::ProgramController* _controller;
};

class ParameterControlService : public ParameterControllerClient
{
public:
    ParameterControlService(sushi::control::SushiControl* controller) : _controller(controller->parameter_controller()) {}

    ~ParameterControlService() = default;

    void GetTrackParameters(const sushi_rpc::TrackIdentifier& request, sushi_rpc::ParameterInfoListResponse& response) override;
    void GetProcessorParameters(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::ParameterInfoListResponse& response) override;
    void GetParameterId(const sushi_rpc::ParameterIdRequest& request, sushi_rpc::ParameterIdentifierResponse& response) override;
    void GetParameterInfo(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::ParameterInfoResponse& response) override;
    void GetParameterValue(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::FloatResponse& response) override;
    void GetParameterValueInDomain(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::FloatResponse& response) override;
    void GetParameterValueAsString(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::StringResponse& response) override;
    void SetParameterValue(const sushi_rpc::ParameterValue& request, sushi_rpc::CommandResponse& response) override;
    void GetTrackProperties(const sushi_rpc::TrackIdentifier& request, sushi_rpc::PropertyInfoListResponse& response) override;
    void GetProcessorProperties(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::PropertyInfoListResponse& response) override;
    void GetPropertyId(const sushi_rpc::PropertyIdRequest& request, sushi_rpc::PropertyIdentifierResponse& response) override;
    void GetPropertyInfo(const sushi_rpc::PropertyIdentifier& request, sushi_rpc::PropertyInfoResponse& response) override;
    void GetPropertyValue(const sushi_rpc::PropertyIdentifier& request, sushi_rpc::StringResponse& response) override;
    void SetPropertyValue(const sushi_rpc::PropertyValue& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::ParameterController* _controller;
};

class MidiControlService : public MidiControllerClient
{
public:
    MidiControlService(sushi::control::SushiControl* controller) : _controller(controller->midi_controller()) {}

    ~MidiControlService() = default;

    void GetInputPorts(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetOutputPorts(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetAllKbdInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::MidiKbdConnectionList& response) override;
    void GetAllKbdOutputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::MidiKbdConnectionList& response) override;
    void GetAllCCInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::MidiCCConnectionList& response) override;
    void GetAllPCInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::MidiPCConnectionList& response) override;
    void GetCCInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::MidiCCConnectionListResponse& response) override;
    void GetPCInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::MidiPCConnectionListResponse& response) override;
    void GetMidiClockOutputEnabled(const sushi_rpc::GenericIntValue& request, sushi_rpc::GenericBoolValue& response) override;
    void SetMidiClockOutputEnabled(const sushi_rpc::MidiClockSetRequest& request, sushi_rpc::CommandResponse& response) override;
    void ConnectKbdInputToTrack(const sushi_rpc::MidiKbdConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectKbdOutputFromTrack(const sushi_rpc::MidiKbdConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectCCToParameter(const sushi_rpc::MidiCCConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectPCToProcessor(const sushi_rpc::MidiPCConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectKbdInput(const sushi_rpc::MidiKbdConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectKbdOutput(const sushi_rpc::MidiKbdConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectCC(const sushi_rpc::MidiCCConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectPC(const sushi_rpc::MidiPCConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllCCFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllPCFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::MidiController* _controller;
};

class AudioRoutingControlService : public AudioRoutingControllerClient
{
public:
    AudioRoutingControlService(sushi::control::SushiControl* controller) : _controller(controller->audio_routing_controller()) {}

    ~AudioRoutingControlService() = default;

    void GetAllInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::AudioConnectionList& response) override;
    void GetAllOutputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::AudioConnectionList& response) override;
    void GetInputConnectionsForTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::AudioConnectionListResponse& response) override;
    void GetOutputConnectionsForTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::AudioConnectionListResponse& response) override;
    void ConnectInputChannelToTrack(const sushi_rpc::AudioConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectOutputChannelFromTrack(const sushi_rpc::AudioConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectInput(const sushi_rpc::AudioConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectOutput(const sushi_rpc::AudioConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllInputsFromTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllOutputFromTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::GenericVoidValue& response) override;
    void DisconnectAllOutputsFromTrack(const sushi_rpc::TrackIdentifier& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::AudioRoutingController* _controller;
};

class CvGateControlService : public CvGateControllerClient
{
public:
    CvGateControlService(sushi::control::SushiControl* controller) : _controller(controller->cv_gate_controller()) {}

    ~CvGateControlService() = default;

    void GetCvInputChannelCount(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetCvOutputChannelCount(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetAllCvInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CvConnectionList& response) override;
    void GetAllCvOutputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CvConnectionList& response) override;
    void GetAllGateInputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GateConnectionList& response) override;
    void GetAllGateOutputConnections(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GateConnectionList& response) override;
    void GetCvInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CvConnectionListResponse& response) override;
    void GetCvOutputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CvConnectionListResponse& response) override;
    void GetGateInputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::GateConnectionListResponse& response) override;
    void GetGateOutputConnectionsForProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::GateConnectionListResponse& response) override;
    void ConnectCvInputToParameter(const sushi_rpc::CvConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectCvOutputFromParameter(const sushi_rpc::CvConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectGateInputToProcessor(const sushi_rpc::GateConnection& request, sushi_rpc::CommandResponse& response) override;
    void ConnectGateOutputFromProcessor(const sushi_rpc::GateConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectCvInput(const sushi_rpc::CvConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectCvOutput(const sushi_rpc::CvConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectGateInput(const sushi_rpc::GateConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectGateOutput(const sushi_rpc::GateConnection& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllCvInputsFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllCvOutputsFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllGateInputsFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisconnectAllGateOutputsFromProcessor(const sushi_rpc::ProcessorIdentifier& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::CvGateController* _controller;
};

class OscControlService : public OscControllerClient
{
public:
    OscControlService(sushi::control::SushiControl* controller) : _controller(controller->osc_controller()) {}

    ~OscControlService() = default;

    void GetSendIP(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericStringValue& response) override;
    void GetSendPort(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetReceivePort(const sushi_rpc::GenericVoidValue& request, sushi_rpc::GenericIntValue& response) override;
    void GetEnabledParameterOutputs(const sushi_rpc::GenericVoidValue& request, sushi_rpc::OscParameterOutputList& response) override;
    void EnableOutputForParameter(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void DisableOutputForParameter(const sushi_rpc::ParameterIdentifier& request, sushi_rpc::CommandResponse& response) override;
    void EnableAllOutput(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CommandResponse& response) override;
    void DisableAllOutput(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::OscController* _controller;
};

class SessionControlService : public SessionControllerClient
{
public:
    SessionControlService(sushi::control::SushiControl* controller) : _controller(controller->session_controller()) {}

    ~SessionControlService() = default;

    void SaveSession(const sushi_rpc::GenericVoidValue& request, sushi_rpc::SessionState& response) override;
    void RestoreSession(const sushi_rpc::SessionState& request, sushi_rpc::CommandResponse& response) override;

private:
    sushi::control::SessionController* _controller;
};

class NotificationControlService : public NotificationControllerClient, private sushi::control::ControlListener
{
public:
    NotificationControlService(sushi::control::SushiControl* controller);

    ~NotificationControlService();

    bool start(zmq::context_t& context, const std::string& socket);

    void stop();

    void notification(const sushi::control::ControlNotification* notification) override;

    void SubscribeToTransportChanges(const sushi_rpc::GenericVoidValue& request, sushi_rpc::TransportUpdate& response) override;
    void SubscribeToEngineCpuTimingUpdates(const sushi_rpc::GenericVoidValue& request, sushi_rpc::CpuTimings& response) override;
    void SubscribeToTrackChanges(const sushi_rpc::GenericVoidValue& request, sushi_rpc::TrackUpdate& response) override;
    void SubscribeToProcessorChanges(const sushi_rpc::GenericVoidValue& request, sushi_rpc::ProcessorUpdate& response) override;
    void SubscribeToParameterUpdates(const sushi_rpc::ParameterNotificationBlocklist& request, sushi_rpc::ParameterUpdate& response) override;
    void SubscribeToPropertyUpdates(const sushi_rpc::PropertyNotificationBlocklist& request, sushi_rpc::PropertyValue& response) override;
    void SubscribeToAsyncCommandUpdates(const sushi_rpc::GenericVoidValue& request, sushi_rpc::AsyncCommandResponse& response) override;

private:
    void _forward_transport_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_cpu_timing_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_track_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_processor_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_parameter_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_property_notification_to_subscribers(const sushi::control::ControlNotification* notification);
    void _forward_async_command_notification_to_subscribers(const sushi::control::ControlNotification* notification);

    void _send(sushi_ipc::Message command, grpc::protobuf::Message& request);

    zmq::socket_t _socket;
    sushi::control::SushiControl* _controller;

    bool _transport_notifications_enabled{false};
    bool _cpu_timing_notifications_enabled{false};
    bool _track_notifications_enabled{false};
    bool _processor_notifications_enabled{false};
    bool _parameter_notifications_enabled{false};
    bool _property_notifications_enabled{false};
    bool _async_notifications_enabled{false};
};

}


#endif //SUSHI_LIBRARY_ZMQ_CONTROL_SERVICE_H
