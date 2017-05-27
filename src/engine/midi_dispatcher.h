/**
 * @Brief Handles translation of midi to internal events and midi routing
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_MIDI_DISPATCHER_H
#define SUSHI_MIDI_DISPATCHER_H

#include <string>
#include <map>
#include <array>
#include <vector>
#include "engine/engine.h"
#include "library/constants.h"
#include "library/midi_decoder.h"
#include "library/plugin_events.h"
#include "library/processor.h"

namespace sushi {
namespace engine {
/* Forward declaration to avoid circular dependancy */
namespace midi_dispatcher {

struct Connection
{
    ObjectId target;
    ObjectId parameter;
    float min_range;
    float max_range;
};

enum class MidiDispatcherStatus
{
    OK,
    INVALID_MIDI_INPUT,
    INVALID_CHAIN_NAME,
    INVALID_PROCESSOR,
    INVALID_PARAMETER
};
class MidiDispatcher
{
    MIND_DECLARE_NON_COPYABLE(MidiDispatcher);

public:
    MidiDispatcher(BaseEngine* engine) : _engine(engine) {}

    ~MidiDispatcher() {}

    void set_midi_input_ports(int channels)
    {
        _midi_inputs = channels;
    }

    void set_midi_output_ports(int channels)
    {
        _midi_outputs = channels;
    }

    /**
     * @brief Connects a midi control change message to a given parameter.
     *        Eventually you should be able to set range, curve etc here.
     * @brief midi_input Index to the registered midi output.
     * @param processor The processor target
     * @param parameter The parameter to map to
     * @param cc_no The cc id to use
     * @param min_range Minimum range for this controller
     * @param max_range Maximum range for this controller
     * @param channel If not OMNI, only the given channel will be connected.
     * @return true if successfully forwarded midi message
     */
    MidiDispatcherStatus connect_cc_to_parameter(int midi_input,
                                 const std::string &processor_name,
                                 const std::string &parameter_name,
                                 int cc_no,
                                 float min_range,
                                 float max_range,
                                 int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect a midi input to a track/processor chain
     *        Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param chain_no The track/plugin chain to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return
     */
    MidiDispatcherStatus connect_kb_to_track(int midi_input,
                             const std::string &chain_name,
                             int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Clears all connections made with connect_kb_to_track
     *        and connect_cc_to_parameter.
     */
    void clear_connections();

    /**
     * @brief Process a raw midi message and send it of according to the
     *        configured connections.
     * @param input Index of the originating midi port.
     * @param offset Offset from the start of the current chunk in samples.
     * @param data Pointer to the raw midi message.
     * @param size Length of data in bytes.
     */
    void process_midi(int input, int offset, const uint8_t* data, size_t size);

private:

    std::map<int, std::array<std::vector<Connection>, midi::MidiChannel::OMNI + 1>> _kb_routes;
    std::map<int, std::array<std::array<std::vector<Connection>, midi::MidiChannel::OMNI + 1>, midi::MAX_CONTROLLER_NO + 1>> _cc_routes;
    int _midi_inputs{0};
    int _midi_outputs{0};

    BaseEngine* _engine;
};

} // end namespace midi_dispatcher
}
} // end namespace sushi

#endif //SUSHI_MIDI_DISPATCHER_H
