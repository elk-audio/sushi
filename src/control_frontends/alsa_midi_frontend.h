/**
 * @brief Alsa midi frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */
#ifndef SUSHI_ALSA_MIDI_FRONTEND_H
#define SUSHI_ALSA_MIDI_FRONTEND_H

#include <thread>
#include <atomic>

#include <alsa/asoundlib.h>

#include "base_midi_frontend.h"

namespace sushi {
namespace midi_frontend {

class AlsaMidiFrontend : public BaseMidiFrontend
{
public:
    AlsaMidiFrontend(midi_dispatcher::MidiDispatcher* dispatcher);

    ~AlsaMidiFrontend();

    bool init() override;

    void run() override;

    void stop() override;

private:

    void                        _worker_function();
    std::thread                 _worker;
    std::atomic<bool>           _running;
    snd_seq_t*                  _seq_handle;
    int                         _input_midi_port;
    snd_midi_event_t*           _seq_parser;
    std::unique_ptr<uint8_t>    _midi_buffer;
};

} // end namespace midi_frontend
} // end namespace sushi
#endif //SUSHI_BASE_MIDI_FRONTEND_H

#ifndef SUSHI_ALSA_MIDI_FRONTEND_H_H
#define SUSHI_ALSA_MIDI_FRONTEND_H_H

#endif //SUSHI_ALSA_MIDI_FRONTEND_H_H
