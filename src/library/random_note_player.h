/**
 * @Brief Test object for ejecting random note events into a queue
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_RANDOM_NOTE_PLAYER_H
#define SUSHI_RANDOM_NOTE_PLAYER_H

#include "library/plugin_events.h"
#include "library/event_fifo.h"

namespace sushi {
namespace dev_util {

/**
 * @brief Test function for pushing random midi note messages to the event queue.
 *        Produces eerie electro acoustic patterns that dont sound half bad.
 */
int random_note_player(EventFifo* queue, bool* loop)
{
    std::deque<int> notes;
    while(*loop)
    {
        int notes_to_play = std::rand() % 4;
        for (int i=0; i < notes_to_play; ++i)
        {
            int note = std::rand() % 127;
            int vel = std::rand() % 127;
            auto event = new KeyboardEvent(EventType::NOTE_ON, "sampler_0_r", i, note, static_cast<float>(vel) / 127.0f);
            queue->push(event);
            notes.push_front(note);
        }

        int notes_to_remove = std::rand() % 2;
        if ((std::rand() % 16) == 15)
        {
            notes_to_remove = static_cast<int>(notes.size()); // Occasionally clear everything
        }
        for (int i=0; i < notes_to_remove; ++i)
        {
            auto event = new KeyboardEvent(EventType::NOTE_OFF, "sampler_0_r", 0, notes.back(), 1);
            queue->push(event);
            notes.pop_back();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() % 2000));
    }
    return 0;
}


} // end namespace dev_util
} // end namespace sushi



#endif //SUSHI_RANDOM_NOTE_PLAYER_H
