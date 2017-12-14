/**
 * @Brief Interface class for receiving midi data
 * @copyright MIND Music Labs AB, Stockholm
 *
 */
#ifndef SUSHI_MIDI_RECEIVER_H
#define SUSHI_MIDI_RECEIVER_H

#include "library/types.h"

namespace sushi {
namespace midi_receiver {

class MidiReceiver
{
public:
    virtual void send_midi(int port, const uint8_t* data, size_t size, int64_t timestamp) = 0;
};


} // end namespace midi_dispatcher
} // end namespace sushi

#endif //SUSHI_MIDI_RECEIVER_H

