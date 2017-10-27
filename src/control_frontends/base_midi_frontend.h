/**
 * @brief Base class for midi frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */
#ifndef SUSHI_BASE_MIDI_FRONTEND_H
#define SUSHI_BASE_MIDI_FRONTEND_H

#include "engine/midi_dispatcher.h"

namespace sushi {
// TODO - Investigate how to get rid of this fwd declaration
namespace midi_dispatcher{ class MidiDispatcher;}
namespace midi_frontend {

class BaseMidiFrontend
{
public:
    BaseMidiFrontend(midi_dispatcher::MidiDispatcher* dispatcher) : _dispatcher(dispatcher) {}

    virtual ~BaseMidiFrontend() {};

    virtual bool init() = 0;

    virtual void run() = 0;

    virtual void stop() = 0;

    virtual void send_midi(int input, int offset, const uint8_t* data, uint64_t timestamp) = 0;

protected:
    midi_dispatcher::MidiDispatcher* _dispatcher;
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_BASE_MIDI_FRONTEND_H
