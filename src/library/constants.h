/**
 * @Brief Compile time constants
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_CONSTANTS_H
#define SUSHI_CONSTANTS_H

/* The number of samples to process in one chunk. It is defined as a
compile time constant to give more room for optimizations */
static constexpr int AUDIO_CHUNK_SIZE = 64;

/* Use in class declaration to disallow copying of this class.
 * Note that this marks copy constructor and assignment operator
 * as deleted and hence their r-value counterparts are not generated.
 *
 * In order to make a class moveable but still non-copyable, implement
 * a move constructor and move assignment operator. Default copy
 * constructor will then not be generated. Usage of this macro is
 * strictly not needed to make the class non-copyable. But can still
 * be used for readability.
 */
#define MIND_DECLARE_NON_COPYABLE(type) type(const type& other) = delete; \
                                        type& operator=(const type&) = delete;


#endif //SUSHI_CONSTANTS_H
