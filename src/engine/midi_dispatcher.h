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
#include <unordered_map>

#include "library/constants.h"
#include "library/midi_decoder.h"
#include "library/plugin_events.h"
#include "library/processor.h"

namespace sushi {
namespace engine {
/* Forward declaration to avoid circular dependancy */
class BaseEngine;
namespace midi_dispatcher {

struct Connection
{
    std::string target{""};
    std::string parameter{""};
    float min_range{0};
    float max_range{0};
};

class MidiDispatcher
{
    MIND_DECLARE_NON_COPYABLE(MidiDispatcher);

public:
    MidiDispatcher(BaseEngine* engine) : _engine(engine)
    {
        std::unordered_multimap<int, Connection> map;
        _kb_routes_by_channel.insert(std::pair<int, std::unordered_multimap<int, Connection>>(0, map));
        _cc_routes.insert(std::pair<int, std::unordered_multimap<int, Connection>>(0, map));

    }

    ~MidiDispatcher()
    {}

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
    bool connect_cc_to_parameter(int midi_input,
                                 const std::string &processor_id,
                                 const std::string &parameter_id,
                                 int cc_no,
                                 int min_range,
                                 int max_range,
                                 int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect a midi input to a track/processor chain
     *        Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param chain_no The track/plugin chain to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return
     */
    bool connect_kb_to_track(int midi_input,
                             const std::string &chain_id,
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


    // TODO eventually replace the channel index with vectors/arrays for faster lookup.
    // midi keyboard data connections indexed by input port
    std::unordered_multimap<int, Connection> _kb_routes;
    // midi keyboard data connections indexed by input port/channel
    std::map<int, std::unordered_multimap<int, Connection>> _kb_routes_by_channel;

    // midi cc connections indexed by input port/cc no
    std::map<int, std::unordered_multimap<int, Connection>> _cc_routes;
    // midi cc connections indexed by input port/channel/cc no
    std::map<int, std::map<int, std::unordered_multimap<int, Connection>>> _cc_routes_by_channel;
    BaseEngine* _engine;
};

typedef  std::unordered_multimap<int, Connection>::const_iterator ConnectionIter;

/**
 * @brief Helper function to look up iterators matching a route.
 */
const std::pair<ConnectionIter, ConnectionIter> get_route_iters_from_nested_map(int key_1, int key_2,
                                                                                const std::map<int, std::unordered_multimap<int, Connection>> &map);
/**
 * @brief Helper function to look up iterators matching a route.
 */
const std::pair<ConnectionIter, ConnectionIter> get_route_iters_from_double_nested_map(int key_1, int key_2, int key_3,
                                                                                       const std::map<int, std::map<int, std::unordered_multimap<int, Connection>>> &map);


} // end namespace midi_dispatcher
}
} // end namespace sushi

#endif //SUSHI_MIDI_DISPATCHER_H
