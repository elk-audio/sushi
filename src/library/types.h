/**
 * @Brief General types and typedefs not suitable to put elsewhere
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_TYPES_H
#define SUSHI_TYPES_H

#include <array>

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
typedef int64_t MicroTime;


constexpr size_t MIDI_DATA_BYTE_SIZE = 4;
/**
 * @brief Convenience type for passing midi messages by value
 */
typedef std::array<uint8_t, 4> MidiDataByte;
static_assert(sizeof(MidiDataByte) == MIDI_DATA_BYTE_SIZE, "");

} // end namespace sushi

#endif //SUSHI_TYPES_H
