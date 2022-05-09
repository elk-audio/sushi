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
 * @Brief General types and typedefs not suitable to put elsewhere
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TYPES_H
#define SUSHI_TYPES_H

#include <array>
#include <cstdint>
#include <cstddef>

namespace sushi {

/**
 * @brief General struct for passing opaque binary data in events or parameters/properties
 */
struct BlobData
{
    int      size{0};
    uint8_t* data{nullptr};
};

constexpr size_t MIDI_DATA_BYTE_SIZE = 4;
/**
 * @brief Convenience type for passing midi messages by value
 */
typedef std::array<uint8_t, MIDI_DATA_BYTE_SIZE> MidiDataByte;
static_assert(sizeof(MidiDataByte) == MIDI_DATA_BYTE_SIZE);

/**
 * @brief Struct to represent a defined time signature
 */
struct TimeSignature
{
    int numerator;
    int denominator;
};

inline bool operator==(const TimeSignature& lhs, const TimeSignature& rhs)
{
    return (lhs.denominator == rhs.denominator && lhs.numerator == rhs.numerator);
}

inline bool operator!=(const TimeSignature& lhs, const TimeSignature& rhs)
{
    return ! (lhs == rhs);
}

/**
 * @brief baseclass for objects that can returned from a realtime thread for destruction
 */
class RtDeletable
{
public:
    virtual ~RtDeletable();
};

/**
 * @brief Wrapper for using  native and standard library types with RtDeletable
 */
template <typename T>
struct RtDeletableWrapper : public RtDeletable
{
public:
    explicit RtDeletableWrapper(T data) : _data(data) {}
    ~RtDeletableWrapper() override = default;

    T& data() {return _data;}
    const T& data() const {return _data;}

private:
    T _data;
};

} // namespace sushi

#endif //SUSHI_TYPES_H
