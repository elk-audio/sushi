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
 * @brief Midi i/o plugin example with a simple arpeggiator
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ARPEGGIATOR_PLUGIN_H
#define SUSHI_ARPEGGIATOR_PLUGIN_H

#include <vector>

#include "library/internal_plugin.h"

namespace sushi {
namespace arpeggiator_plugin {

/**
 * @brief Simple arpeggiator module with only 1 mode (up)
 *        The last held note will be remembered and played indefinitely
 */
class Arpeggiator
{
public:
    Arpeggiator();

    /**
     * @brief Add a note to the list of notes playing
     * @param note The note number of the note to add
     */
    void add_note(int note);

    /**
     * @brief Remove this note from the list of notes playing
     * @param note The note number to remove
     */
    void remove_note(int note);

    /**
     * @brief Set the playing range of the arpeggiator
     * @param range The range in octaves
     */
    void set_range(int range);

    /**
     * @brief Call sequentially to get the full arpeggio sequence
     * @return The note number of the next note in the sequence
     */
    int next_note();

private:
    std::vector<int> _notes;
    int              _range{2};
    int              _octave_idx{0};
    int              _note_idx{-1};
    bool             _hold{true};
};


class ArpeggiatorPlugin : public InternalPlugin, public UidHelper<ArpeggiatorPlugin>
{
public:
    ArpeggiatorPlugin(HostControl host_control);

    ~ArpeggiatorPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

    static std::string_view static_uid();

private:
    float                _sample_rate;
    double               _last_note_beat{0};
    int                  _current_note{0};

    IntParameterValue*   _range_parameter;
    Arpeggiator          _arp;
};

}// namespace sample_player_plugin
}// namespace sushi

#endif //SUSHI_ARPEGGIATOR_PLUGIN_H
