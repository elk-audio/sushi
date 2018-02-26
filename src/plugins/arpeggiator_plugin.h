/**
 * @brief Midi i/o plugin example with a simple arpeggiator
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_ARPEGGIATOR_PLUGIN_H
#define SUSHI_ARPEGGIATOR_PLUGIN_H

#include <vector>

#include "library/internal_plugin.h"

namespace sushi {
namespace arpeggiator_plugin {

static const std::string DEFAULT_NAME = "sushi.testing.arpeggiator";
static const std::string DEFAULT_LABEL = "Arpeggiator";

constexpr int MAX_OCTAVE = 2;
constexpr int MAX_ARP_NOTES = 8;
constexpr int START_NOTE = 48;

/**
 * @brief Simple arpeggiator module with only 1 mode (up)
 *        The last helt note will be remembered and played indefinitely
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
    int  next_note();
private:

    std::vector<int> _notes;
    int              _range{2};
    int              _octave_idx{0};
    int              _note_idx{-1};
    bool             _hold{true};
};


class ArpeggiatorPlugin : public InternalPlugin
{
public:
    ArpeggiatorPlugin(HostControl host_control);

    ~ArpeggiatorPlugin() {}

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_bypassed(bool bypassed) override;

    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer&/*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

private:

    float                _sample_rate;
    float                _sample_counter{0.0f};
    int                  _current_note{0};

    FloatParameterValue* _tempo_parameter;
    IntParameterValue*   _range_parameter;
    Arpeggiator          _arp;
};


float samples_per_note(float note_fraction, float tempo, float samplerate);

}// namespace sample_player_plugin
}// namespace sushi

#endif //SUSHI_ARPEGGIATOR_PLUGIN_H
