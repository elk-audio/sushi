/**
 * @Brief General types not suitable to put elsewhere
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_TYPES_H
#define SUSHI_TYPES_H

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

} // end namespace sushi

#endif //SUSHI_TYPES_H
