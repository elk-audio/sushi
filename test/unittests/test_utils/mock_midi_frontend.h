/**
 * @brief Mock object for MidiFrontend
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MOCK_MIDI_FRONTEND_H
#define SUSHI_MOCK_MIDI_FRONTEND_H

#include <gmock/gmock.h>

#include "engine/midi_dispatcher.h"

using namespace sushi;

class MockMidiFrontend : public midi_frontend::BaseMidiFrontend
{
public:
    MockMidiFrontend(midi_receiver::MidiReceiver* receiver) : BaseMidiFrontend(receiver) {}

    MOCK_METHOD(bool,
                init,
                (),
                (override));

    MOCK_METHOD(void,
                run,
                (),
                (override));

    MOCK_METHOD(void,
                stop,
                (),
                (override));

    MOCK_METHOD(void,
                send_midi,
                (int, MidiDataByte, Time),
                (override));

};

#endif //SUSHI_MOCK_MIDI_FRONTEND_H
