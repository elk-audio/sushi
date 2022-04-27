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
 * @brief Handles translation of midi to internal events and midi routing
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MIDI_DISPATCHER_H
#define SUSHI_MIDI_DISPATCHER_H

#include <string>
#include <map>
#include <array>
#include <vector>
#include <mutex>

#include "library/constants.h"
#include "library/types.h"
#include "library/midi_decoder.h"
#include "library/event.h"
#include "library/processor.h"
#include "control_frontends/base_midi_frontend.h"
#include "library/event_interface.h"

namespace sushi {
namespace engine {
class BaseProcessorContainer;
}

namespace midi_dispatcher {

struct InputConnection
{
    // TODO: This can be track_id, if the InputConnection is member of KbdInputConnection.
    // It can also be processor_id, if the InputConnection is member of CCInputConnection.
    // Disambiguating would be safer.
    ObjectId target;
    ObjectId parameter;
    float min_range;
    float max_range;
    bool relative;
    uint8_t virtual_abs_value;
};

struct OutputConnection
{
    int channel;
    int output;
    int cc_number;
    float min_range;
    float max_range;
};

enum class MidiDispatcherStatus
{
    OK,
    INVALID_MIDI_INPUT,
    INVALID_MIDI_OUTPUT,
    INVALID_PROCESSOR,
    INVALID_TRACK,
    INVALID_PARAMETER,
    INVAlID_CHANNEL
};

// These structs are only used for returning query data, to midi_controller.
struct CCInputConnection
{
    InputConnection input_connection;
    int channel;
    int port;
    int cc;
};

struct PCInputConnection
{
    int processor_id;
    int channel;
    int port;
};

struct KbdInputConnection
{
    InputConnection input_connection;
    int port;
    int channel;
    bool raw_midi;
};

struct KbdOutputConnection
{
    ObjectId track_id;
    int port;
    int channel;
};

class MidiDispatcher : public EventPoster, public midi_receiver::MidiReceiver
{
    SUSHI_DECLARE_NON_COPYABLE(MidiDispatcher);

public:
    MidiDispatcher(dispatcher::BaseEventDispatcher* event_dispatcher);

    virtual ~MidiDispatcher();

    void set_frontend(midi_frontend::BaseMidiFrontend* frontend)
    {
        _frontend = frontend;
    }

    /**
     * @brief Sets the number of midi input ports.
     * Not intended to be called dynamically, only once during creation.
     * @param ports number of input ports.
     */
    void set_midi_inputs(int no_inputs)
    {
        _midi_inputs = no_inputs;
    }

    /**
     * @brief Returns the number of midi input ports.
     */
    int get_midi_inputs() const
    {
        return _midi_inputs;
    }

    /**
     * @brief Sets the number of midi output ports.
     * Not intended to be called dynamically, only once during creation.
     * @param ports number of output ports.
     */
    void set_midi_outputs(int no_outputs);

    /**
     * @brief Returns the number of midi output ports.
     */
    int get_midi_outputs() const
    {
        return _midi_outputs;
    }

    /**
     * @brief Connects a midi control change message to a given parameter.
     *        Eventually you should be able to set range, curve etc here.
     * @param midi_input Index to the registered midi output.
     * @param processor The processor target
     * @param parameter The parameter to map to
     * @param cc_no The cc id to use
     * @param min_range Minimum range for this controller
     * @param max_range Maximum range for this controller
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully forwarded midi message
     */
    MidiDispatcherStatus connect_cc_to_parameter(int midi_input,
                                                 ObjectId processor_id,
                                                 ObjectId parameter_id,
                                                 int cc_no,
                                                 float min_range,
                                                 float max_range,
                                                 bool use_relative_mode,
                                                 int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnects a midi control change message from a given parameter.
     * @param midi_input Index to the registered midi output.
     * @param processor_id The processor target
     * @param parameter The parameter mapped
     * @param cc_no The cc id to use
     * @return OK if successfully disconnected
     */
    MidiDispatcherStatus disconnect_cc_from_parameter(int midi_input,
                                                      ObjectId processor_id,
                                                      int cc_no,
                                                      int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnects all midi control change messages from a given processor's parameters.
     * @param processor_id The processor target
     * @return OK if successfully disconnected
     */
    MidiDispatcherStatus disconnect_all_cc_from_processor(ObjectId processor_id);

    /**
     * @brief Returns a vector of CC_InputConnections for all the Midi Control Change input connections defined.
     * @return A vector of CC_InputConnections.
     */
    std::vector<CCInputConnection> get_all_cc_input_connections();

    /**
     * @brief Returns a vector of CC_InputConnections for all the Midi Control Change input connections
     * defined for the processor id passed as input.
     * @param The id of the processor for which the connections are queried.
     * @return A vector of CC_InputConnections.
     */
    std::vector<CCInputConnection> get_cc_input_connections_for_processor(int processor_id);

    /**
     * @brief Connects midi program change messages to a processor.
     * @param midi_input Index to the registered midi output.
     * @param processor The processor target
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully forwarded midi message
     */
    MidiDispatcherStatus connect_pc_to_processor(int midi_input,
                                                 ObjectId processor_id,
                                                 int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnects midi program change messages from a processor.
     * @param midi_input Index to the registered midi output.
     * @param processor The processor target
     * @return OK if successfully disconnected
     */
    MidiDispatcherStatus disconnect_pc_from_processor(int midi_input,
                                                      ObjectId processor_id,
                                                      int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnects all midi program change messages from a given processor.
     * @param processor_id The processor target
     * @return OK if successfully disconnected
     */
    MidiDispatcherStatus disconnect_all_pc_from_processor(ObjectId processor_id);

    /**
     * @brief Returns a vector of PC_InputConnections for all the Midi Program Change input connections defined.
     * @return A vector of PC_InputConnections.
     */
    std::vector<PCInputConnection> get_all_pc_input_connections();

    /**
     * @brief Returns a vector of PC_InputConnections for all the Midi Program Change input connections
     * defined for the processor id passed as input.
     * @param The id of the processor for which the connections are queried.
     * @return A vector of PC_InputConnections.
     */
    std::vector<PCInputConnection> get_pc_input_connections_for_processor(int processor_id);

    /**
     * @brief Connect a midi input to a track
     *        Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_kb_to_track(int midi_input,
                                             ObjectId track_id,
                                             int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnect a midi input from a track
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully disconnected from the track, error status otherwise
     */
    MidiDispatcherStatus disconnect_kb_from_track(int midi_input,
                                                  ObjectId track_id,
                                                  int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Returns a vector of Kbd_InputConnections for all the Midi Keyboard input connections defined.
     * @return A vector of Kbd_InputConnections.
     */
    std::vector<KbdInputConnection> get_all_kb_input_connections();

    /**
     * @brief Connect a midi input to a track and send unprocessed
     *        Midi data to it. Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_raw_midi_to_track(int midi_input,
                                                   ObjectId track_id,
                                                   int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Disconnect a midi input from a track.
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track to disconnect
     * @return OK if successfully disconnected the track, error status otherwise
     */
    MidiDispatcherStatus disconnect_raw_midi_from_track(int midi_input,
                                                        ObjectId track_id,
                                                        int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect midi kb data from a track to a given midi output
     * @param midi_output Index of the midi out
     * @param track_name The track/processor track from where the data originates
     * @param channel Which channel nr to output the data on
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_track_to_output(int midi_output,
                                                 ObjectId track_id,
                                                 int channel);

    /**
     * @brief Disconnect midi kb data from a track to a given midi output
     * @param midi_output Index of the midi out
     * @param track_name The track/processor track from where the data originates
     * @return OK if successfully disconnected the track, error status otherwise
     */
    MidiDispatcherStatus disconnect_track_from_output(int midi_output,
                                                      ObjectId track_id,
                                                      int channel);

    /**
     * @brief Returns a vector of Kbd_OutputConnections for all the Midi Keyboard output connections defined.
     * @return A vector of Kbd_OutputConnections.
     */
    std::vector<KbdOutputConnection> get_all_kb_output_connections();

    /**
     * @brief Enable or disable sending of midi clock through an output
     * @param enabled If true enables sending of midi clock messages (24ppqn),
     *        and start and stop messages.
     * @param midi_output The midi output to configure
     * @return
     */
    MidiDispatcherStatus enable_midi_clock(bool enabled, int midi_output);

    /**
     * @brief Returns whether midi clock output is enabled for a particular midi output
     * @param midi_output The midi output port
     * @return true if the selected midi output is configured to send midi clock, false otherwise
     */
    bool midi_clock_enabled(int midi_output);

    /**
     * @brief Process a raw midi message and send it of according to the
     *        configured connections.
     * @param port Index of the originating midi port.
     * @param data Pointer to the raw midi message.
     * @param size Length of data in bytes.
     * @param timestamp timestamp of the midi event
     */
    void send_midi(int port, MidiDataByte data, Time timestamp) override;

    /* Inherited from EventPoster */
    int process(Event* /*event*/) override;

    /**
     * @brief The unique id of this poster.
     * @return
     */
    int poster_id() override {return EventPosterId::MIDI_DISPATCHER;}

private:

    bool _handle_engine_notification(const EngineNotificationEvent* event);
    bool _handle_audio_graph_notification(const AudioGraphNotificationEvent* event);
    bool _handle_transport_notification(const PlayingModeNotificationEvent* event);
    bool _handle_tick_notification(const EngineTimingTickNotificationEvent* event);

    std::vector<CCInputConnection> _get_cc_input_connections(std::optional<int> processor_id_filter);
    std::vector<PCInputConnection> _get_pc_input_connections(std::optional<int> processor_id_filter);

    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _kb_routes_in;
    std::map<ObjectId, std::vector<OutputConnection>>  _kb_routes_out;
    std::map<int, std::array<std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>, midi::MAX_CONTROLLER_NO + 1>> _cc_routes;
    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _pc_routes;
    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _raw_routes_in;
    int _midi_inputs{0};
    int _midi_outputs{0};

    std::mutex _kb_routes_in_lock;
    std::mutex _kb_routes_out_lock;
    std::mutex _cc_routes_lock;
    std::mutex _pc_routes_lock;
    std::mutex _raw_routes_in_lock;

    std::vector<int> _enabled_clock_out;

    midi_frontend::BaseMidiFrontend* _frontend;
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // end namespace midi_dispatcher
} // end namespace sushi

#endif //SUSHI_MIDI_DISPATCHER_H
