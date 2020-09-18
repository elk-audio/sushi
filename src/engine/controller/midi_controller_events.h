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
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MIDI_CONTROLLER_EVENTS_H
#define SUSHI_MIDI_CONTROLLER_EVENTS_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"

namespace sushi {

namespace engine {
namespace controller_impl {

class MidiControllerEvent : public EngineEvent
{
public:

protected:
    explicit MidiControllerEvent(Time timestamp,
                                 midi_dispatcher::MidiDispatcher* midi_dispatcher) : EngineEvent(timestamp),
                                                                                     _midi_dispatcher(midi_dispatcher) {}

    midi_dispatcher::MidiDispatcher* _midi_dispatcher;
};

class KbdInputToTrackConnectionEvent : public MidiControllerEvent
{
public:
    enum class Action {
        Connect,
        Disconnect
    };

    KbdInputToTrackConnectionEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                   ObjectId track_id,
                                   ext::MidiChannel channel,
                                   int port,
                                   bool raw_midi,
                                   Action action,
                                   Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                     _track_id(track_id),
                                                     _channel(channel),
                                                     _port(port),
                                                     _raw_midi(raw_midi),
                                                     _action(action) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _track_id;
    ext::MidiChannel _channel;
    int _port;
    bool _raw_midi;
    Action _action;
};

class KbdOutputToTrackConnectionEvent : public MidiControllerEvent
{
public:
    enum class Action {
        Connect,
        Disconnect
    };

    KbdOutputToTrackConnectionEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                    ObjectId track_id,
                                    ext::MidiChannel channel,
                                    int port,
                                    Action action,
                                    Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                      _track_id(track_id),
                                                      _channel(channel),
                                                      _port(port),
                                                      _action(action) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _track_id;
    ext::MidiChannel _channel;
    int _port;
    Action _action;
};

class ConnectCCToParameterEvent : public MidiControllerEvent
{
public:
    ConnectCCToParameterEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                              ObjectId processor_id,
                              const std::string& parameter_name,
                              ext::MidiChannel channel,
                              int port,
                              int cc_number,
                              float min_range,
                              float max_range,
                              bool relative_mode,
                              Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                _processor_id(processor_id),
                                                _parameter_name(parameter_name),
                                                _channel(channel),
                                                _port(port),
                                                _cc_number(cc_number),
                                                _min_range(min_range),
                                                _max_range(max_range),
                                                _relative_mode(relative_mode) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _processor_id;
    std::string _parameter_name;
    ext::MidiChannel _channel;
    int _port;
    int _cc_number;
    float _min_range;
    float _max_range;
    bool _relative_mode;
};

class DisconnectCCEvent : public MidiControllerEvent
{
public:
    DisconnectCCEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                      ObjectId processor_id,
                      ext::MidiChannel channel,
                      int port,
                      int cc_number,
                      Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                        _processor_id(processor_id),
                                        _channel(channel),
                                        _port(port),
                                        _cc_number(cc_number) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _processor_id;
    ext::MidiChannel _channel;
    int _port;
    int _cc_number;
};

class PCToProcessorConnectionEvent : public MidiControllerEvent
{
public:
    enum class Action {
        Connect,
        Disconnect
    };

    PCToProcessorConnectionEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                 ObjectId processor_id,
                                 ext::MidiChannel channel,
                                 int port,
                                 Action action,
                                 Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                   _processor_id(processor_id),
                                                   _channel(channel),
                                                   _port(port),
                                                   _action(action) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _processor_id;
    ext::MidiChannel _channel;
    int _port;
    Action _action;
};

class DisconnectAllCCFromProcessorEvent : public MidiControllerEvent
{
public:
    DisconnectAllCCFromProcessorEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                      ObjectId processor_id,
                                      Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                        _processor_id(processor_id) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _processor_id;
};

class DisconnectAllPCFromProcessorEvent : public MidiControllerEvent
{
public:
    DisconnectAllPCFromProcessorEvent(midi_dispatcher::MidiDispatcher* midi_dispatcher,
                                      ObjectId processor_id,
                                      Time timestamp) : MidiControllerEvent(timestamp, midi_dispatcher),
                                                        _processor_id(processor_id) {}

    int execute(engine::BaseEngine* /*engine*/) const override;

private:
    ObjectId _processor_id;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_MIDI_CONTROLLER_EVENTS_H
