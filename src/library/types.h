/**
 * @Brief General types and typedefs not suitable to put elsewhere
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_TYPES_H
#define SUSHI_TYPES_H

#include <array>
#include <chrono>

namespace sushi {

/**
 * @brief General struct for passing opaque binary data in events or parameters/properties
 */
struct BlobData
{
    int   size{0};
    uint8_t* data{nullptr};
};

/**
 * @brief Type used for timestamps with micro second granularity
 */
typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds> Time;

/**
 * @brief Convenience shorthand for setting timestamp to 0, i.e. process event as soon as possible.
 */
constexpr Time PROCESS_NOW = std::chrono::time_point<std::chrono::steady_clock, std::chrono::microseconds>::min();


constexpr size_t MIDI_DATA_BYTE_SIZE = 4;
/**
 * @brief Convenience type for passing midi messages by value
 */
typedef std::array<uint8_t, MIDI_DATA_BYTE_SIZE> MidiDataByte;
static_assert(sizeof(MidiDataByte) == MIDI_DATA_BYTE_SIZE, "");

} // end namespace sushi

#endif //SUSHI_TYPES_H
