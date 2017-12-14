/**
 * @brief Base class for midi frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */
#ifndef SUSHI_BASE_MIDI_FRONTEND_H
#define SUSHI_BASE_MIDI_FRONTEND_H

#include "engine/midi_receiver.h"

namespace sushi {
namespace midi_frontend {

class BaseMidiFrontend
{
public:
    BaseMidiFrontend(midi_receiver::MidiReceiver* receiver) : _receiver(receiver) {}

    virtual ~BaseMidiFrontend() {};

    virtual bool init() = 0;

    virtual void run() = 0;

    virtual void stop() = 0;

    virtual void send_midi(int input, const uint8_t* data, int64_t timestamp) = 0;

protected:
    midi_receiver::MidiReceiver* _receiver;
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_BASE_MIDI_FRONTEND_H
