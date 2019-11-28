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
 * @brief Test object for ejecting random note events into a queue
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_RANDOM_NOTE_PLAYER_H
#define SUSHI_RANDOM_NOTE_PLAYER_H

#include "library/rt_events.h"
#include "library/rt_event_fifo.h"

namespace sushi {
namespace dev_util {

/**
 * @brief Test function for pushing random midi note messages to the event queue.
 *        Produces eerie electro acoustic patterns that dont sound half bad.
 */
static int random_note_player(EventFifo* queue, bool* loop)
{
    std::deque<int> notes;
    while(*loop)
    {
        int notes_to_play = std::rand() % 4;
        for (int i=0; i < notes_to_play; ++i)
        {
            int note = std::rand() % 127;
            int vel = std::rand() % 127;
            auto event = new KeyboardEvent(EventType::NOTE_ON, "sampler_0_r", i, 0, note, static_cast<float>(vel) / 127.0f);
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
