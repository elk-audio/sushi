/**
 * @Brief Circular Buffer FIFO that can return a contiguous buffer to
 *        its elements. This is useful when dealing with APIs that require
 *        this constraint in their interface, e.g. when passing events
 *        to VsT API call processEvents(VstEvents* events)
 * @warning Not thread-safe! It's ok with the current architecture where
 *          Processor::process_event(..) is called in the real-time thread
 *          before processing.
 *
 *          FIFO policy is a circular buffer which just overwrites old events,
 *          signaling the producer in case of overflow.
 *
 *          The only read operation completely flushes the buffer. If we need
 *          to have a more flexible structure that acts more like a normal FIFO,
 *          we should check the Bip Buffer:
 *          https://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist
 *
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_CONTIGUOUS_READ_FIFO_H
#define SUSHI_CONTIGUOUS_READ_FIFO_H

#include "constants.h"

namespace sushi {

template<typename T, int capacity>
class ContiguousReadFIFO
{
public:
    struct ReadBuffer
    {
        ReadBuffer(T* data_, int n_items_) :
            data{data_},
            n_items(n_items_)
        {}

        T*  data;
        int n_items;
    };

    MIND_DECLARE_NON_COPYABLE(ContiguousReadFIFO)
    /**
     *  @brief Allocate an empty queue with given capacity
     */
    ContiguousReadFIFO()
    {
        _data = new T[capacity];
    }

    ~ContiguousReadFIFO()
    {
        delete[] _data;
    }

    /**
     *  @brief Push an element to the FIFO.
     *  @return false if overflow happened during the operation
     */
    bool push(T item)
    {
        bool res = !_limit_reached;
        _data[_write_idx] = item;
        _write_idx++;
        if (!_limit_reached)
        {
            _size++;
        }
        if (_write_idx == capacity)
        {
            // Reached end of buffer: wrap pointer and next call will signal overflow
            _write_idx = 0;
            _limit_reached = true;
        }

        return res;
    }

    /**
     * @brief Pass a pointer to a contiguous buffer.
     *        You should process _all_ the returned values before any subsequent
     *        call to push(), as the internal buffer is flushed after the call.
     * @return Read-only buffer with data pointer and size
     */
    ReadBuffer flush()
    {
        auto res = ReadBuffer(_data, _size);
        _size = 0;
        _write_idx = 0;
        _limit_reached = false;

        return res;
    }

private:
    bool _limit_reached{false};
    int _size{0};
    int _write_idx{0};
    T* _data;
};

} // namespace sushi

#endif //SUSHI_CONTIGUOUS_READ_FIFO_H
